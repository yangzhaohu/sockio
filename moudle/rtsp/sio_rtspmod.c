#include "sio_rtspmod.h"
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include "sio_common.h"
#include "sio_def.h"
#include "moudle/sio_locate.h"
#include "http_parser/http_parser.h"
#include "sio_regex.h"
#include "sio_rtspdev.h"
#include "sio_rtpchn.h"
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

struct sio_rtsp_conn
{
    struct http_parser parser;
    struct sio_rtsp_buffer buf;
    struct sio_rtpchn *rtpchn;
    struct sio_rtspdev rtdev;
    enum sio_rtsp_type type;

    sio_str_t field;
    unsigned int seq;
    unsigned int rtp;
    unsigned int rtcp;
    char addrname[64];
    sio_str_t body;

    unsigned int complete:1; // packet complete
};

struct sio_rtspmod
{
    struct sio_locate *locat;
    struct sio_rtpool *rtpool;
};

static struct sio_rtspmod g_rtspmod;

#define sio_rtspmod_get_locat() g_rtspmod.locat
#define sio_rtspmod_get_rtpool() g_rtspmod.rtpool

#define sio_rtspdev_get(type) g_rtspdev[type]

static void sio_rtspmod_live_record(struct sio_rtpchn *rtpchn, const char *data, int len);

int sio_rtspmod_name(const char **name)
{
    *name = "sio_rtsp_core_module";
    return 0;
}

int sio_rtspmod_type()
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
    struct sio_locate *locat = sio_rtspmod_get_locat();

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
    sprintf(buffer, "RTSP/1.0 404 OK\r\n"
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
    if (rconn->type == SIO_RTSPTYPE_VOD) {
        // open self rtdev
        struct sio_rtspdev *rtdev = &rconn->rtdev;
        *rtdev = sio_rtspdev_get(SIO_RTSPTYPE_VOD);
        rtdev->dev = rtdev->open(rconn->addrname);

        // get rtdev describe
        rtdev->describe(rtdev->dev, &scri);

    } else if (rconn->type == SIO_RTSPTYPE_LIVE) {
        // first find exist live stream
        struct sio_rtpool *rtpool = sio_rtspmod_get_rtpool();
        struct sio_rtspdev *rtdev = NULL;
        int ret = sio_rtpool_find(rtpool, rconn->addrname, (void **)&rtdev);
        SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
            SIO_LOGE("rtpdev %s not found\n", rconn->addrname),
            sio_rtspmod_response_404(sock),
            sio_socket_shutdown(sock, SIO_SOCK_SHUTRDWR));

        // get live stream describe
        rtdev->describe(rtdev->dev, &scri);

        // open self rtdev
        rtdev = &rconn->rtdev;
        *rtdev = sio_rtspdev_get(SIO_RTSPTYPE_LIVE);
        rtdev->dev = rtdev->open(rconn->addrname);
    }

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
    struct sio_rtspdev *rtdev = &rconn->rtdev;

    // open rtdev
    *rtdev = sio_rtspdev_get(SIO_RTSPTYPE_LIVERECORD);
    rtdev->dev = rtdev->open(NULL);
    rtdev->setdescribe(rtdev->dev, rconn->body.data, rconn->body.length);

    rconn->type = SIO_RTSPTYPE_LIVERECORD;

    struct sio_rtpool *rtpool = sio_rtspmod_get_rtpool();
    int ret = sio_rtpool_add(rtpool, rconn->addrname, rtdev);
    SIO_COND_CHECK_CALLOPS(ret == -1,
            SIO_LOGE("rtpdev %s add failed\n", rconn->addrname));

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
    struct sio_rtspdev *rtdev = &rconn->rtdev;

    struct sio_rtpchn *rtpchn = sio_rtpchn_overudp_open(sock, rconn->rtp, rconn->rtcp);
    rconn->rtpchn = rtpchn;

    // set send rtpchn
    if (rconn->type == SIO_RTSPTYPE_VOD) {
        rtdev->add_senddst(rtdev->dev, rtpchn);
    } else if (rconn->type == SIO_RTSPTYPE_LIVE) {
        struct sio_rtpool *rtpool = sio_rtspmod_get_rtpool();
        sio_rtpool_find(rtpool, rconn->addrname, (void **)&rtdev);
        rtdev->add_senddst(rtdev->dev, rtpchn);
    }

    unsigned int rtpport = sio_rtpchn_chnrtp(rtpchn);
    unsigned int rtcpport = sio_rtpchn_chnrtcp(rtpchn);

    char buffer[1024] = { 0 };
    sprintf(buffer, "RTSP/1.0 200 OK\r\n"
                    "CSeq: %u\r\n"
                    "Transport: RTP/AVP;unicast;client_port=%u-%u;server_port=%u-%u\r\n"
                    "Session: 66334873\r\n"
                    "\r\n", rconn->seq, rconn->rtp, rconn->rtcp, rtpport, rtcpport);

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

    return 0;
}

static inline
int sio_rtspmod_record_response(struct sio_socket *sock)
{
    struct sio_rtsp_conn *rconn = sio_rtspmod_get_rtspconn_from_conn(sock);

    union sio_rtpchn_opt opt = { .private = rconn };
    sio_rtpchn_setopt(rconn->rtpchn, SIO_RTPCHN_PRIVATE, &opt);

    opt.ops.rtpack = sio_rtspmod_live_record;
    sio_rtpchn_setopt(rconn->rtpchn, SIO_RTPCHN_OPS, &opt);

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

    return rconn;
}

static inline
void sio_rtspmod_rtspconn_destory(struct sio_rtsp_conn *rconn)
{
    free(rconn->buf.buf);
    free(rconn);
}

static void sio_rtspmod_live_record(struct sio_rtpchn *rtpchn, const char *data, int len)
{
    union sio_rtpchn_opt opt = { 0 };
    sio_rtpchn_getopt(rtpchn, SIO_RTPCHN_PRIVATE, &opt);

    struct sio_rtsp_conn *rconn = opt.private;
    struct sio_rtspdev *rtdev = &rconn->rtdev;

    rtdev->record(rtdev->dev, data, len);
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

    l = buf->last - buf->pos;
    l = http_parser_execute(&rconn->parser, &g_http_parser_cb, buf->pos, l);

    if (rconn->complete != 1) {
        buf->pos += l;
        return 0;
    }

    rconn->complete = 0;
    buf->last = buf->pos = buf->buf; // reset

    int method = rconn->parser.method;
    switch (method) {
    case HTTP_OPTIONS:
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
int sio_rtspmod_socket_writeable(struct sio_socket *sock)
{
    return 0;
}

static inline
int sio_rtspmod_socket_closeable(struct sio_socket *sock)
{
    struct sio_rtsp_conn *rconn = sio_rtspmod_get_rtspconn_from_conn(sock);

    if (rconn->type == SIO_RTSPTYPE_LIVERECORD) {
        struct sio_rtpool *rtpool = sio_rtspmod_get_rtpool();
        sio_rtpool_del(rtpool, rconn->addrname);
    }

    // rtdev close
    struct sio_rtspdev *rtdev = &rconn->rtdev;
    if (rtdev->close) {
        rtdev->close(rtdev->dev);
    }

    sio_rtpchn_close(rconn->rtpchn);

    sio_rtspmod_rtspconn_destory(rconn);

    sio_socket_destory(sock);

    return 0;
}

int sio_rtspmod_create(void)
{
    struct sio_locate *locat = sio_locate_create();
    SIO_COND_CHECK_RETURN_VAL(!locat, -1);

    struct sio_rtpool *rtpool = sio_rtpool_create();
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!rtpool, -1,
        sio_locate_destory(locat));

    g_rtspmod.locat = locat;
    g_rtspmod.rtpool = rtpool;

    return 0;
}

int sio_rtspmod_setlocat(const struct sio_location *locations, int size)
{
    struct sio_locate *locate = sio_rtspmod_get_locat();
    for (int i = 0; i < size; i++) {
        if (locations[i].type < SIO_LOCATE_RTSP_VOD) {
            continue;
        }
        sio_locate_add(locate, &locations[i]);
    }

    return 0;
}

int sio_rtspmod_newconn(struct sio_server *server)
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

    ret = sio_server_socket_mplex(server, sock);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        sio_socket_destory(sock));
    
    struct sio_rtsp_conn *rconn = sio_rtspmod_rtspconn_create(NULL);
    opt.private = rconn;
    sio_socket_setopt(sock, SIO_SOCK_PRIVATE, &opt);

    return 0;
}

int sio_rtspmod_destory(void)
{
    return 0;
}