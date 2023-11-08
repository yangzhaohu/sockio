#include "sio_rtspmod.h"
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include "sio_common.h"
#include "sio_server.h"
#include "moudle/sio_locate.h"
#include "http_parser/http_parser.h"
#include "sio_regex.h"
#include "sio_rtspdev.h"
#include "sio_rtspipe.h"
#include "sio_rtpvod.h"
#include "sio_rtplive.h"
#include "sio_rtpool.h"
#include "sio_log.h"

struct sio_rtsp_buffer
{
    char *buf;
    char *pos;
    char *last;
    char *end;
};

const struct sio_rtspdev g_rtspdev[] =
{
    [SIO_RTSPTYPE_VOD] = {
        .open = sio_rtpvod_open,
        .play = sio_rtpvod_play,
        .describe = sio_rtpvod_get_describe,
        .add_senddst = sio_rtpvod_add_senddst,
        .close = sio_rtpvod_close
    },
    [SIO_RTSPTYPE_LIVE] = {
        .open = sio_rtspdev_open_default,
        .play = sio_rtspdev_play_default,
        .describe = sio_rtspdev_describe_default,
        .add_senddst = sio_rtspdev_add_senddst_default,
        .close = sio_rtspdev_close_default
    },
    [SIO_RTSPTYPE_LIVERECORD] = {
        .open = sio_rtplive_open,
        .record = sio_rtplive_record,
        .setdescribe = sio_rtplive_set_describe,
        .describe = sio_rtplive_get_describe,
        .add_senddst = sio_rtplive_add_senddst,
        .rm_senddst = sio_rtplive_rm_senddst,
        .close = sio_rtplive_close
    }
};

struct sio_rtsp_rtp
{
    char *data;
    unsigned short len;
    enum sio_rtspipe_t pipe;
    enum sio_rtspchn_t chn;
};

enum sio_rtsp_packst
{
    SIO_RTSP_PACKST_INITIAL,
    SIO_RTSP_PACKST_HTTPACK,
    SIO_RTSP_PACKST_RTPACKSIZE,
    SIO_RTSP_PACKST_RTPACKAGE
};

struct sio_rtsp_conn
{
    struct http_parser parser;
    struct sio_rtspmod *mod;
    struct sio_rtsp_buffer buf;
    struct sio_rtspipe *rtpipe;
    struct sio_rtspdev rtdev;
    enum sio_rtsp_type type;

    sio_str_t field;
    unsigned int seq;
    unsigned int rtp;
    unsigned int rtcp;
    char addrname[64];
    sio_str_t body;

    unsigned int setup:1;
    unsigned int overtcp:1;
    unsigned int complete:1; // packet complete

    enum sio_rtsp_packst packst;
    struct sio_rtsp_rtp package;
};

struct sio_rtspmod
{
    struct sio_locate *locate;
    struct sio_rtpool *rtpool;
};

// static struct sio_rtspmod g_rtspmod;

// #define sio_rtspmod_get_locat() g_rtspmod.locat
// #define sio_rtspmod_get_rtpool() g_rtspmod.rtpool

#define sio_rtspdev_get(type) g_rtspdev[type]

static void sio_rtspmod_live_record(struct sio_rtspipe *rtpipe, const char *data, int len);

int sio_rtspmod_name(const char **name)
{
    *name = "sio_rtsp_core_module";
    return 0;
}

int sio_rtspmod_type(void)
{
    return SIO_SUBMOD_RTSP;
}

int sio_rtspmod_version(const char **version)
{
    *version = "0.0.1";
    return 0;
}

static inline
struct sio_rtsp_conn *sio_rtspmod_get_rtspconn_from_conn(struct sio_socket *sock)
{
    union sio_socket_opt opt = { 0 };
    sio_socket_getopt(sock, SIO_SOCK_PRIVATE, &opt);

    struct sio_rtsp_conn *rconn = opt.private;
    return rconn;
}

static inline
int sio_rtsp_packet_url(http_parser *parser, const char *at, size_t len)
{
    struct sio_rtsp_conn *rconn = (struct sio_rtsp_conn *)parser;
    struct sio_locate *locat = rconn->mod->locate;

    if (rconn->addrname[0] != 0) {
        return 0;
    }

    struct sio_location location = { 0 };
    int matchpos = sio_locate_match(locat, at, len, &location);
    if (matchpos == -1) {
        return 0;
    }

    if (location.type == SIO_LOCATE_RTSP_VOD) {
        rconn->type = SIO_RTSPTYPE_VOD;
    } else if (location.type == SIO_LOCATE_RTSP_LIVE) {
        rconn->type = SIO_RTSPTYPE_LIVE;
    }

    int pos = matchpos;
    for (; pos < len; pos++) {
        if (at[pos] == '/') {
            break;
        }
    }

    memcpy(rconn->addrname, at + matchpos, pos - matchpos);

    return 0;
}

static inline
int sio_rtsp_packet_head_field(http_parser *parser, const char *at, size_t len)
{
    struct sio_rtsp_conn *rconn = (struct sio_rtsp_conn *)parser;
    sio_str_t *field = &rconn->field;
    field->data = at;
    field->length = len;

    return 0;
}

static inline
int sio_rtsp_packet_head_field_value(http_parser *parser, const char *at, size_t len)
{
    struct sio_rtsp_conn *rconn = (struct sio_rtsp_conn *)parser;
    sio_str_t *field = &rconn->field;

    if (strncmp(field->data, "CSeq", strlen("CSeq")) == 0) {
        char port[len + 1];
        port[len] = 0;
        memcpy(port, at, len);
        sscanf(port, "%u", &rconn->seq);
    } else if (strncmp(field->data, "Transport", strlen("Transport")) == 0) {
        int pos = sio_regex("RTP/AVP/TCP", at, len);
        if (pos >= 0) {
            rconn->overtcp = 1;
        }
        char port[len + 1];
        port[len] = 0;
        memcpy(port, at, len);
        sscanf(port, "%*[^;];%*[^;];%*[^=]=%u-%u", &rconn->rtp, &rconn->rtcp);
    }

    return 0;
}

static inline
int sio_rtsp_packet_header_complete(http_parser *parser)
{
    return 0;
}

static inline
int sio_rtsp_packet_body(http_parser *parser, const char *at, size_t len)
{
    struct sio_rtsp_conn *rconn = (struct sio_rtsp_conn *)parser;

    rconn->body.data = at;
    rconn->body.length = len;

    return 0;
}

static inline
int sio_rtsp_packet_complete(http_parser *parser)
{
    struct sio_rtsp_conn *rconn = (struct sio_rtsp_conn *)parser;
    rconn->complete = 1;

    return 0;
}

static http_parser_settings g_http_parser_cb = {
    .on_url = sio_rtsp_packet_url,
    .on_header_field = sio_rtsp_packet_head_field,
    .on_header_value = sio_rtsp_packet_head_field_value,
    .on_headers_complete = sio_rtsp_packet_header_complete,
    .on_body = sio_rtsp_packet_body,
    .on_message_complete = sio_rtsp_packet_complete,
};

static inline
int sio_rtspmod_response_404(struct sio_socket *sock)
{
    struct sio_rtsp_conn *rconn = sio_rtspmod_get_rtspconn_from_conn(sock);

    char buffer[1024] = { 0 };
    sprintf(buffer, "RTSP/1.0 404 Not Found\r\n"
                    "CSeq: %u\r\n"
                    "\r\n", rconn->seq);

    sio_socket_write(sock, buffer, strlen(buffer));

    return 0;
}

static inline
int sio_rtspmod_options_response(struct sio_socket *sock)
{
    struct sio_rtsp_conn *rconn = sio_rtspmod_get_rtspconn_from_conn(sock);

    char buffer[1024] = { 0 };
    sprintf(buffer, "RTSP/1.0 200 OK\r\n"
                    "CSeq: %u\r\n"
                    "Public: %s\r\n"
                    "\r\n", rconn->seq, "OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, ANNOUNCE, RECORD");

    sio_socket_write(sock, buffer, strlen(buffer));

    return 0;
}

static inline
int sio_rtspmod_describe_response(struct sio_socket *sock)
{
    struct sio_rtsp_conn *rconn = sio_rtspmod_get_rtspconn_from_conn(sock);

    const char *scri = NULL;
    struct sio_rtspdev *rtdev = NULL;
    if (rconn->type == SIO_RTSPTYPE_VOD) {
        rtdev = &rconn->rtdev;
    } else if (rconn->type == SIO_RTSPTYPE_LIVE) {
        // first find exist live stream dev
        struct sio_rtpool *rtpool = rconn->mod->rtpool;
        int ret = sio_rtpool_find(rtpool, rconn->addrname, (void **)&rtdev);
        SIO_COND_CHECK_CALLOPS(ret == -1,
            SIO_LOGE("rtpdev %s not found\n", rconn->addrname));
    }

    int ret = -1;
    if (rtdev) {
        ret = rtdev->describe(rtdev->dev, &scri);
    }

    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        sio_rtspmod_response_404(sock));

    char buffer[1024] = { 0 };
    sprintf(buffer, "RTSP/1.0 200 OK\r\n"
                     "CSeq: %u\r\n"
                     "Content-length: %ld\r\n"
                     "Content-type: application/sdp\r\n"
                     "\r\n", rconn->seq, strlen(scri));

    sio_socket_write(sock, buffer, strlen(buffer));
    sio_socket_write(sock, scri, strlen(scri));

    return 0;
}

static inline
int sio_rtspmod_announce_response(struct sio_socket *sock)
{
    struct sio_rtsp_conn *rconn = sio_rtspmod_get_rtspconn_from_conn(sock);

    char buffer[1024] = { 0 };
    sprintf(buffer, "RTSP/1.0 200 OK\r\n"
                    "CSeq: %u\r\n"
                    "\r\n", rconn->seq);

    sio_socket_write(sock, buffer, strlen(buffer));

    return 0;
}

static inline
unsigned int sio_rtspmod_rtp_timestamp()
{
    struct timeval tv = { 0 };
    gettimeofday(&tv, NULL);
    unsigned int ts = ((tv.tv_sec * 1000) + ((tv.tv_usec + 500) / 1000)) * 90; // 90: _clockRate/1000;

    return ts;
}

static inline
int sio_rtspmod_setup_response(struct sio_socket *sock)
{
    struct sio_rtsp_conn *rconn = sio_rtspmod_get_rtspconn_from_conn(sock);

    struct sio_rtspipe *rtpipe = NULL;
    if (rconn->rtpipe == NULL) {
        if (rconn->overtcp) {
            rtpipe = sio_rtspipe_create(sock, SIO_RTSPIPE_OVERTCP);
        } else {
            rtpipe = sio_rtspipe_create(sock, SIO_RTSPIPE_OVERUDP);
        }
        rconn->rtpipe = rtpipe;
    } else {
        rtpipe = rconn->rtpipe;
    }

    unsigned int rtpport = 0;
    unsigned int rtcpport = 0;
    // first open video chnnel
    if (rconn->setup == 0) {
        sio_rtspipe_open_videochn(rtpipe, rconn->rtp, rconn->rtcp);
        rtpport = sio_rtspipe_getchn(rtpipe, SIO_RTSPIPE_VIDEO, SIO_RTSPCHN_RTP);
        rtcpport = sio_rtspipe_getchn(rtpipe, SIO_RTSPIPE_VIDEO, SIO_RTSPCHN_RTCP);
        rconn->setup = 1;
    } else {
        sio_rtspipe_open_audiochn(rtpipe, rconn->rtp, rconn->rtcp);
        rtpport = sio_rtspipe_getchn(rtpipe, SIO_RTSPIPE_AUDIO, SIO_RTSPCHN_RTP);
        rtcpport = sio_rtspipe_getchn(rtpipe, SIO_RTSPIPE_AUDIO, SIO_RTSPCHN_RTCP);
    }

    char buffer[1024] = { 0 };
    if (rconn->overtcp) {
        sprintf(buffer, "RTSP/1.0 200 OK\r\n"
                        "CSeq: %u\r\n"
                        "Transport: RTP/AVP/TCP;unicast;interleaved=%u-%u\r\n"
                        "Session: 66334873\r\n"
                        "\r\n", rconn->seq, rconn->rtp, rconn->rtcp);
    } else {
        sprintf(buffer, "RTSP/1.0 200 OK\r\n"
                        "CSeq: %u\r\n"
                        "Transport: RTP/AVP;unicast;client_port=%u-%u;server_port=%u-%u\r\n"
                        "Session: 66334873\r\n"
                        "\r\n", rconn->seq, rconn->rtp, rconn->rtcp, rtpport, rtcpport);
    }

    sio_socket_write(sock, buffer, strlen(buffer));

    return 0;
}

static inline
int sio_rtspmod_play_response(struct sio_socket *sock)
{
    struct sio_rtsp_conn *rconn = sio_rtspmod_get_rtspconn_from_conn(sock);
    struct sio_rtspdev *rtdev = &rconn->rtdev;

    rtdev->play(rtdev->dev);

    char buffer[1024] = { 0 };
    sprintf(buffer, "RTSP/1.0 200 OK\r\n"
                    "CSeq: %u\r\n"
                    "Range: npt=0.000-\r\n"
                    "Session: 66334873; timeout=60\r\n"
                    "\r\n", rconn->seq);

    sio_socket_write(sock, buffer, strlen(buffer));

    if (rconn->type == SIO_RTSPTYPE_VOD) {
        rtdev->add_senddst(rtdev->dev, rconn->rtpipe);
    } else if (rconn->type == SIO_RTSPTYPE_LIVE) {
        struct sio_rtpool *rtpool = rconn->mod->rtpool;
        sio_rtpool_find(rtpool, rconn->addrname, (void **)&rtdev);
        rtdev->add_senddst(rtdev->dev, rconn->rtpipe);
    }

    return 0;
}

static inline
int sio_rtspmod_record_response(struct sio_socket *sock)
{
    struct sio_rtsp_conn *rconn = sio_rtspmod_get_rtspconn_from_conn(sock);

    union sio_rtspipe_opt opt = { .private = rconn };
    sio_rtspipe_setopt(rconn->rtpipe, SIO_RTPCHN_PRIVATE, &opt);

    opt.ops.rtpack = sio_rtspmod_live_record;
    sio_rtspipe_setopt(rconn->rtpipe, SIO_RTPCHN_OPS, &opt);

    char buffer[1024] = { 0 };
    sprintf(buffer, "RTSP/1.0 200 OK\r\n"
                    "CSeq: %u\r\n"
                    "Session: 66334873\r\n"
                    "\r\n", rconn->seq);

    sio_socket_write(sock, buffer, strlen(buffer));

    return 0;
}

static inline
int sio_rtspmod_teardown_response(struct sio_socket *sock)
{
    struct sio_rtsp_conn *rconn = sio_rtspmod_get_rtspconn_from_conn(sock);

    char buffer[1024] = { 0 };
    sprintf(buffer, "RTSP/1.0 200 OK\r\n"
                    "CSeq: %u\r\n"
                    "\r\n", rconn->seq);

    sio_socket_write(sock, buffer, strlen(buffer));

    return 0;
}

static inline
int sio_rtspmod_open_playdev(struct sio_rtsp_conn *rconn)
{
    struct sio_rtspdev *rtdev = &rconn->rtdev;
    SIO_COND_CHECK_RETURN_VAL(rtdev->dev, 0);

    if (rconn->type == SIO_RTSPTYPE_VOD) {
        *rtdev = sio_rtspdev_get(SIO_RTSPTYPE_VOD);
    } else if (rconn->type == SIO_RTSPTYPE_LIVE) {
        *rtdev = sio_rtspdev_get(SIO_RTSPTYPE_LIVE);
    }

    rtdev->dev = rtdev->open(rconn->addrname);
    SIO_COND_CHECK_RETURN_VAL(rtdev->dev == NULL, -1);

    return 0;
}

static inline
int sio_rtspmod_open_recorddev(struct sio_rtsp_conn *rconn)
{
    struct sio_rtspdev *rtdev = &rconn->rtdev;
    SIO_COND_CHECK_RETURN_VAL(rtdev->dev, 0);

    // open rtdev
    *rtdev = sio_rtspdev_get(SIO_RTSPTYPE_LIVERECORD);
    rtdev->dev = rtdev->open(NULL);
    rtdev->setdescribe(rtdev->dev, rconn->body.data, rconn->body.length);

    rconn->type = SIO_RTSPTYPE_LIVERECORD;

    struct sio_rtpool *rtpool = rconn->mod->rtpool;
    int ret = sio_rtpool_add(rtpool, rconn->addrname, rtdev);
    SIO_COND_CHECK_CALLOPS(ret == -1,
            SIO_LOGE("rtpdev %s add failed\n", rconn->addrname));

    return 0;
}

static inline
int sio_rtspmod_response(struct sio_socket *sock, int method)
{
    struct sio_rtsp_conn *rconn = sio_rtspmod_get_rtspconn_from_conn(sock);
    int ret = 0;
    switch (method) {
    case HTTP_OPTIONS:
        ret = sio_rtspmod_open_playdev(rconn);
        SIO_COND_CHECK_CALLOPS_BREAK(ret == -1,
            sio_rtspmod_response_404(sock));
        sio_rtspmod_options_response(sock);
        break;

    case HTTP_DESCRIBE:
        sio_rtspmod_describe_response(sock);
        break;

    case HTTP_SETUP:
        sio_rtspmod_setup_response(sock);
        break;

    case HTTP_PLAY:
        sio_rtspmod_play_response(sock);
        break;

    case HTTP_ANNOUNCE:
        ret = sio_rtspmod_open_recorddev(rconn);
        SIO_COND_CHECK_CALLOPS_BREAK(ret == -1,
            sio_rtspmod_response_404(sock));
        sio_rtspmod_announce_response(sock);
        break;

    case HTTP_RECORD:
        sio_rtspmod_record_response(sock);
        break;

    case HTTP_TEARDOWN:
        sio_rtspmod_teardown_response(sock);
        break;

    default:
        break;
    }

    return 0;
}

static inline
struct sio_rtsp_conn *sio_rtspmod_rtspconn_create(struct sio_socket *sock)
{
    struct sio_rtsp_conn *rconn = malloc(sizeof(struct sio_rtsp_conn));
    SIO_COND_CHECK_RETURN_VAL(!rconn, NULL);

    memset(rconn, 0, sizeof(struct sio_rtsp_conn));

    char *mem = malloc(sizeof(char) * 40960);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!mem, NULL,
        free(rconn));

    memset(mem, 0, sizeof(char) * 40960);

    struct sio_rtsp_buffer *buf = &rconn->buf;
    buf->buf = mem;
    buf->pos = mem;
    buf->last = mem;
    buf->end = mem + sizeof(char) * 40960;

    http_parser_init(&rconn->parser, HTTP_REQUEST);

    rconn->packst = SIO_RTSP_PACKST_INITIAL;

    return rconn;
}

static inline
void sio_rtspmod_rtspconn_destory(struct sio_rtsp_conn *rconn)
{
    free(rconn->buf.buf);
    free(rconn);
}

static inline
void sio_rtspmod_live_record2(struct sio_rtsp_conn *rcon, const char *data, int len)
{
    struct sio_rtspdev *rtdev = &rcon->rtdev;

    if (rtdev->record) {
        rtdev->record(rtdev->dev, data, len);
    }
}

static void sio_rtspmod_live_record(struct sio_rtspipe *rtpipe, const char *data, int len)
{
    union sio_rtspipe_opt opt = { 0 };
    sio_rtspipe_getopt(rtpipe, SIO_RTPCHN_PRIVATE, &opt);

    struct sio_rtsp_conn *rconn = opt.private;
    struct sio_rtspdev *rtdev = &rconn->rtdev;

    rtdev->record(rtdev->dev, data, len);
}

static inline
int sio_rtspmod_rtp_valid(struct sio_rtsp_conn *rcon, unsigned char chn)
{
    struct sio_rtsp_rtp *package = &rcon->package;
    struct sio_rtspipe *rtpipe = rcon->rtpipe;
    if (sio_rtspipe_peerchn(rtpipe, SIO_RTSPIPE_VIDEO, SIO_RTSPCHN_RTP) == chn) {
        package->pipe = SIO_RTSPIPE_VIDEO;
        package->chn = SIO_RTSPCHN_RTP;
        return 0;
    } else if (sio_rtspipe_peerchn(rtpipe, SIO_RTSPIPE_VIDEO, SIO_RTSPCHN_RTCP) == chn) {
        package->pipe = SIO_RTSPIPE_VIDEO;
        package->chn = SIO_RTSPCHN_RTCP;
        return 0;
    }else if (sio_rtspipe_peerchn(rtpipe, SIO_RTSPIPE_AUDIO, SIO_RTSPCHN_RTP) == chn) {
        package->pipe = SIO_RTSPIPE_AUDIO;
        package->chn = SIO_RTSPCHN_RTP;
        return 0;
    } else if (sio_rtspipe_peerchn(rtpipe, SIO_RTSPIPE_AUDIO, SIO_RTSPCHN_RTCP) == chn) {
        package->pipe = SIO_RTSPIPE_AUDIO;
        package->chn = SIO_RTSPCHN_RTCP;
        return 0;
    }

    return -1;
}

static inline
int sio_rtspmod_buffer_move(struct sio_rtsp_buffer *buf)
{
    int l = buf->last - buf->pos;
    SIO_COND_CHECK_RETURN_VAL(buf->pos == buf->buf, l);

    if (l > 0) {
        memcpy(buf->buf, buf->pos, l);
        buf->buf[l] = 0;
    }
    buf->pos = buf->buf;
    buf->last = buf->pos + l;

    return l;
}

static inline
int sio_rtspmod_is_httpreq(const char *data, int len)
{
    struct http_parser parser;
    http_parser_init(&parser, HTTP_REQUEST);
    http_parser_settings http_parser_cb = { 0 };

    http_parser_execute(&parser, &http_parser_cb, data, len);

    return parser.http_errno == HPE_OK ? 0 : -1;
}

static inline
int sio_rtspmod_rtsp_unpack(struct sio_rtsp_conn *rcon, struct sio_socket *sock)
{
    struct sio_rtsp_buffer *buf = &rcon->buf;
    struct sio_rtsp_rtp *package = &rcon->package;

    int l = buf->last - buf->pos;
    SIO_COND_CHECK_RETURN_VAL(l < 5, -1); // data not enough

    while (l) {
        switch (rcon->packst) {
        case SIO_RTSP_PACKST_INITIAL:
            if (sio_rtspmod_is_httpreq(buf->pos, l) == 0) {
                l = sio_rtspmod_buffer_move(buf);
                rcon->packst = SIO_RTSP_PACKST_HTTPACK;
            } else if((unsigned char)buf->pos[0] == '$' &&
                sio_rtspmod_rtp_valid(rcon, (unsigned char)buf->pos[1]) == 0) {
                rcon->packst = SIO_RTSP_PACKST_RTPACKSIZE;
            } else {
                if (l > 0) { // find backward
                    buf->pos++;
                    l = buf->last - buf->pos;
                }
            }
            break;
        case SIO_RTSP_PACKST_HTTPACK:
        {
            unsigned int len = http_parser_execute(&rcon->parser, &g_http_parser_cb, buf->pos, l);
            if (rcon->complete != 1) {
                buf->pos += len;
                return 0;
            } else {
                buf->pos += len;
            }

            // complete
            rcon->complete = 0;
            l = sio_rtspmod_buffer_move(buf);
            sio_rtspmod_response(sock, rcon->parser.method);
            rcon->packst = SIO_RTSP_PACKST_INITIAL;
            break;
        }
        case SIO_RTSP_PACKST_RTPACKSIZE:
        {
            unsigned short *len = (unsigned short *)&buf->pos[2];
            package->len = ntohs(*len);
            rcon->packst = SIO_RTSP_PACKST_RTPACKAGE;
            break;
        }
        case SIO_RTSP_PACKST_RTPACKAGE:
            if (l < package->len + 4) {
                return 0;
            }
            sio_rtspmod_live_record2(rcon, buf->pos + 4, package->len);
            buf->pos += package->len + 4;
            package->len = 0;
            // l = buf->last - buf->pos; // update
            l = sio_rtspmod_buffer_move(buf);
            rcon->packst = SIO_RTSP_PACKST_INITIAL;
            break;

        default:
            break;
        }
    }

    return 0;
}

static inline
int sio_rtspmod_socket_readable(struct sio_socket *sock)
{
    struct sio_rtsp_conn *rconn = sio_rtspmod_get_rtspconn_from_conn(sock);
    struct sio_rtsp_buffer *buf = &rconn->buf;

    int l = buf->end - buf->last;
    int len = sio_socket_read(sock, buf->last, l);
    SIO_COND_CHECK_RETURN_VAL(len <= 0, 0);
    buf->last += len;

    sio_rtspmod_rtsp_unpack(rconn, sock);

    return 0;
}

static inline
int sio_rtspmod_socket_writeable(struct sio_socket *sock)
{
    return 0;
}

static inline
int sio_rtspmod_socket_closeable(struct sio_socket *sock)
{
    struct sio_rtsp_conn *rconn = sio_rtspmod_get_rtspconn_from_conn(sock);

    if (rconn->type == SIO_RTSPTYPE_LIVERECORD) {
        struct sio_rtpool *rtpool = rconn->mod->rtpool;
        sio_rtpool_del(rtpool, rconn->addrname);
    }

    // rtdev close
    struct sio_rtspdev *rtdev = &rconn->rtdev;
    if (rtdev->close) {
        rtdev->close(rtdev->dev);
    }

    sio_rtspipe_close(rconn->rtpipe);

    sio_rtspmod_rtspconn_destory(rconn);

    sio_socket_destory(sock);

    return 0;
}

sio_submod_t sio_rtspmod_create(void)
{
    struct sio_rtspmod *mod = malloc(sizeof(struct sio_rtspmod));
    SIO_COND_CHECK_RETURN_VAL(!mod, NULL);

    memset(mod, 0, sizeof(struct sio_rtspmod));

    struct sio_locate *locate = sio_locate_create();
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!locate, NULL,
        free(mod));

    struct sio_rtpool *rtpool = sio_rtpool_create();
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!rtpool, NULL,
        sio_locate_destory(locate),
        free(mod));

    mod->locate = locate;
    mod->rtpool = rtpool;

    return mod;
}

int sio_rtspmod_setlocate(sio_submod_t mod, const struct sio_location *locations, int size)
{
    struct sio_locate *locate = ((struct sio_rtspmod *)mod)->locate;
    for (int i = 0; i < size; i++) {
        if (locations[i].type < SIO_LOCATE_RTSP_VOD) {
            continue;
        }
        sio_locate_add(locate, &locations[i]);
    }

    return 0;
}

int sio_rtspmod_newconn(sio_submod_t mod, struct sio_server *server)
{
    struct sio_socket *sock = sio_socket_create(SIO_SOCK_TCP, NULL);
    union sio_socket_opt opt = {
        .ops.readable = sio_rtspmod_socket_readable,
        .ops.writeable = sio_rtspmod_socket_writeable,
        .ops.closeable = sio_rtspmod_socket_closeable
    };
    sio_socket_setopt(sock, SIO_SOCK_OPS, &opt);

    opt.nonblock = 1;
    sio_socket_setopt(sock, SIO_SOCK_NONBLOCK, &opt);

    int ret = sio_server_accept(server, sock);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        sio_socket_destory(sock));

    struct sio_rtsp_conn *rconn = sio_rtspmod_rtspconn_create(NULL);
    rconn->mod = mod;

    opt.private = rconn;
    sio_socket_setopt(sock, SIO_SOCK_PRIVATE, &opt);

    ret = sio_server_socket_mplex(server, sock);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        sio_socket_destory(sock));

    return 0;
}

int sio_rtspmod_destory(sio_submod_t mod)
{
    struct sio_locate *locate = ((struct sio_rtspmod *)mod)->locate;
    sio_locate_destory(locate);

    struct sio_rtpool *rtpool = ((struct sio_rtspmod *)mod)->rtpool;
    sio_rtpool_destory(rtpool);

    free(mod);

    return 0;
}