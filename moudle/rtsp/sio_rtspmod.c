#include "sio_rtspmod.h"
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include "sio_common.h"
#include "sio_def.h"
#include "proto/sio_rtspprot.h"
#include "sio_rtpchn.h"
#include "sio_rtpvod.h"
#include "sio_log.h"

struct sio_rtsp_buffer
{
    char *buf;
    char *pos;
    char *last;
    char *end;
};

struct sio_rtsp_conn
{
    struct sio_rtsp_buffer buf;
    struct sio_rtspprot *prot;
    struct sio_rtpchn *rtpchn;
    struct sio_rtpvod *rtpvod;

    sio_str_t field;
    unsigned int seq;
    unsigned int rtp;
    unsigned int rtcp;

    unsigned int complete:1; // packet complete
    unsigned int live:1;
};

struct sio_rtspmod
{
    int resv;
};

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
struct sio_rtsp_conn *sio_rtspmod_get_rtspconn_from_rtspprot(struct sio_rtspprot *prot)
{
    union sio_rtspprot_opt opt = { 0 };
    sio_rtspprot_getopt(prot, SIO_RTSPPROT_PRIVATE, &opt);

    return opt.private;
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
int sio_rtspmod_protourl(struct sio_rtspprot *prot, const char *at, int len)
{
    // SIO_LOGE("url: %.*s\n", len, at);
    return 0;
}

static inline
int sio_rtspmod_protofiled(struct sio_rtspprot *prot, const char *at, int len)
{
    // SIO_LOGE("field: %.*s\n", len, at);
    struct sio_rtsp_conn *rconn = sio_rtspmod_get_rtspconn_from_rtspprot(prot);
    sio_str_t *field = &rconn->field;
    field->data = at;
    field->length = len;
    return 0;
}

static inline
int sio_rtspmod_protovalue(struct sio_rtspprot *prot, const char *at, int len)
{
    // SIO_LOGE("value: %.*s\n", len, at);
    struct sio_rtsp_conn *rconn = sio_rtspmod_get_rtspconn_from_rtspprot(prot);
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
int sio_rtspmod_protocomplete(struct sio_rtspprot *prot)
{
    struct sio_rtsp_conn *rconn = sio_rtspmod_get_rtspconn_from_rtspprot(prot);
    rconn->complete = 1;
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

    rconn->rtpvod = sio_rtpvod_create();
    sio_rtpvod_open(rconn->rtpvod, "test.jpg");

    rconn->live = 0;

    char scri[512] = { 0 };
    sprintf(scri, "v=0\r\n"
                  "o=- 91565340853 1 in IP4 127.0.0.1\r\n"
                  "t=0 0\r\n"
                  "a=contol:*\r\n"
                  "m=video 0 RTP/AVP 26\r\n"
                  "a=rtpmap:26 JPEG/90000\r\n"
                  "a=control:track0\r\n");

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

    rconn->live = 1;

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

    struct sio_rtpchn *rtpchn = sio_rtpchn_overudp_open(sock, rconn->rtp, rconn->rtcp);
    rconn->rtpchn = rtpchn;
    sio_rtpvod_attach_rtpchn(rconn->rtpvod, rtpchn);
    sio_rtpvod_start(rconn->rtpvod);

    // struct sio_rtpchn *rtpchn = sio_rtpvod_open_rtpchn(rconn->rtpvod, sock, rconn->rtpchn, rconn->rtcpchn, SIO_RTPCHN_OVER_UDP);
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

    struct sio_rtspprot *prot = sio_rtspprot_create();
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!prot, NULL,
        free(mem),
        free(rconn));
    
    union sio_rtspprot_opt opt = {
        .private = rconn
    };
    sio_rtspprot_setopt(prot, SIO_RTSPPROT_PRIVATE, &opt);

    memset(&opt.ops, 0, sizeof(struct sio_rtspprot_ops));
    opt.ops.prot_url = sio_rtspmod_protourl;
    opt.ops.prot_filed = sio_rtspmod_protofiled;
    opt.ops.prot_value = sio_rtspmod_protovalue;
    opt.ops.prot_complete = sio_rtspmod_protocomplete;
    sio_rtspprot_setopt(prot, SIO_RTSPPROT_OPS, &opt);
    
    rconn->prot = prot;

    return rconn;
}

static inline
void sio_rtspmod_rtspconn_destory(struct sio_rtsp_conn *rconn)
{
    sio_rtspprot_destory(rconn->prot);
    sio_rtpvod_destory(rconn->rtpvod);

    free(rconn->buf.buf);
    free(rconn);
}

static inline
int sio_rtspmod_socket_readable(struct sio_socket *sock)
{
    struct sio_rtsp_conn *rconn = sio_rtspmod_get_rtspconn_from_conn(sock);
    struct sio_rtspprot *prot = rconn->prot;
    struct sio_rtsp_buffer *buf = &rconn->buf;

    int l = buf->end - buf->last;
    int len = sio_socket_read(sock, buf->last, l);
    SIO_COND_CHECK_RETURN_VAL(len <= 0, 0);
    buf->last += len;

    l = buf->last - buf->pos;
    l = sio_rtspprot_process(prot, buf->pos, l);

    if (rconn->complete != 1) {
        buf->pos += l;
        return 0;
    }

    rconn->complete = 0;
    buf->last = buf->pos = buf->buf; // reset

    int method = sio_rtspprot_method(prot);
    switch (method) {
    case 1 << SIO_RTSP_OPTIONS:
        sio_rtspmod_options_response(sock);
        break;
    
    case 1 << SIO_RTSP_DESCRIBE:
        sio_rtspmod_describe_response(sock);
        break;
    
    case 1 << SIO_RTSP_SETUP:
        sio_rtspmod_setup_response(sock);
        break;

    case 1 << SIO_RTSP_PLAY:
        sio_rtspmod_play_response(sock);
        break;

    case 1 << SIO_RTSP_ANNOUNCE:
        sio_rtspmod_announce_response(sock);
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
    sio_rtspmod_rtspconn_destory(rconn);

    sio_socket_destory(sock);

    return 0;
}

int sio_rtspmod_create(void)
{
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