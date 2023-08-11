#include "sio_rtspmod.h"
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include "sio_common.h"
#include "sio_def.h"
#include "proto/sio_rtspprot.h"
#include "jpeg/sio_rtp_jpeg.h"
#include "sio_thread.h"
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
    sio_conn_t conn;
    struct sio_rtsp_buffer buf;

    struct sio_rtspprot *prot;

    sio_str_t field;
    sio_str_t cseq;
    sio_str_t cliport;
    unsigned int complete:1; // packet complete
};

struct sio_rtspmod
{
    int resv;
};

int g_port_rtp = 0;
int g_port_rtcp = 0;

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
struct sio_rtsp_conn *sio_rtspmod_get_rtspconn_from_conn(sio_conn_t conn)
{
    union sio_conn_opt opt = { 0 };
    sio_conn_getopt(conn, SIO_CONN_PRIVATE, &opt);

    struct sio_rtsp_conn *rtspconn = opt.private;
    return rtspconn;
}

static inline
int sio_rtspmod_protobegin(struct sio_rtspprot *prot, const char *at)
{
    return 0;
}

static inline
int sio_rtspmod_protomethod(struct sio_rtspprot *prot, const char *at, int len)
{
    // SIO_LOGE("meth: %.*s\n", len, at);
    return 0;
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
    struct sio_rtsp_conn *rtspconn = sio_rtspmod_get_rtspconn_from_rtspprot(prot);
    sio_str_t *field = &rtspconn->field;
    field->data = at;
    field->length = len;
    return 0;
}

static inline
int sio_rtspmod_protovalue(struct sio_rtspprot *prot, const char *at, int len)
{
    // SIO_LOGE("value: %.*s\n", len, at);
    struct sio_rtsp_conn *rtspconn = sio_rtspmod_get_rtspconn_from_rtspprot(prot);
    sio_str_t *field = &rtspconn->field;

    if (strncmp(field->data, "CSeq", strlen("CSeq")) == 0) {
        sio_str_t *cseq = &rtspconn->cseq;
        cseq->data = at;
        cseq->length = len;
    } else if (strncmp(field->data, "Transport", strlen("Transport")) == 0) {
        sio_str_t *cliport = &rtspconn->cliport;
        cliport->data = at;
        cliport->length = len;
        char port[32] = { 0 };
        memcpy(port, cliport->data, len);
        sscanf(port, "RTP/AVP;unicast;client_port=%d-%d", &g_port_rtp, &g_port_rtcp);
    }
    return 0;
}

static inline
int sio_rtspmod_protocomplete(struct sio_rtspprot *prot)
{
    struct sio_rtsp_conn *rtspconn = sio_rtspmod_get_rtspconn_from_rtspprot(prot);
    rtspconn->complete = 1;
    return 0;
}

static inline
int sio_rtspmod_options_response(sio_conn_t conn)
{
    struct sio_rtsp_conn *rtspconn = sio_rtspmod_get_rtspconn_from_conn(conn);
    sio_str_t *cseq = &rtspconn->cseq;

    char buffer[1024] = { 0 };
    sprintf(buffer, "RTSP/1.0 200 OK\r\n"
                    "CSeq: %.*s\r\n"
                    "Public: %s\r\n"
                    "\r\n", cseq->length, cseq->data, "OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY");

    sio_conn_write(conn, buffer, strlen(buffer));

    return 0;
}

static inline
int sio_rtspmod_describe_response(sio_conn_t conn)
{
    struct sio_rtsp_conn *rtspconn = sio_rtspmod_get_rtspconn_from_conn(conn);
    sio_str_t *cseq = &rtspconn->cseq;

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
                     "CSeq: %.*s\r\n"
                     "Content-length: %ld\r\n"
                     "Content-type: application/sdp\r\n"
                     "\r\n", cseq->length, cseq->data, strlen(scri));

    sio_conn_write(conn, buffer, strlen(buffer));
    sio_conn_write(conn, scri, strlen(scri));

    return 0;
}

sio_conn_t conn_rtp = NULL;
sio_conn_t conn_rtcp = NULL;
static inline
int sio_rtspmod_udpconn_create()
{
    conn_rtp = sio_conn_create(SIO_SOCK_UDP, NULL);
    conn_rtcp = sio_conn_create(SIO_SOCK_UDP, NULL);
    struct sio_socket *sock_rtp = sio_conn_socket_ref(conn_rtp);
    struct sio_socket *sock_rtcp = sio_conn_socket_ref(conn_rtcp);
    struct sio_socket_addr addr = {"127.0.0.1", 56400};
    int ret = sio_socket_bind(sock_rtp, &addr);
    if (ret == -1) {
        SIO_LOGE("rtp %d port bind failed\n", 56400);
    }

    struct sio_socket_addr addr2 = {"127.0.0.1", 56401};
    ret = sio_socket_bind(sock_rtcp, &addr2);
    if (ret == -1) {
        SIO_LOGE("rtcp %d port bind failed\n", 56401);
    }

    return 0;
}

static inline
unsigned int sio_rtspmod_rtp_timestamp()
{
    struct timeval tv = { 0 };
    gettimeofday(&tv, NULL);
    unsigned int ts = ((tv.tv_sec*1000)+((tv.tv_usec+500)/1000))*90; // 90: _clockRate/1000;

    return ts;
}

static inline
void sio_rtspmod_rtp_send(const unsigned char *data, unsigned int len)
{
    struct sio_socket *sock_rtp = sio_conn_socket_ref(conn_rtp);
    struct sio_socket_addr addr = {"127.0.0.1", g_port_rtp};
    sio_socket_writeto(sock_rtp, (const char *)data, len, &addr);
}

void *sio_rtspmod_rtp_routine(void *arg)
{
    FILE *fp = fopen("test.jpg", "rb");
    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    unsigned char *jpegdata = malloc(len);
    fread(jpegdata, 1, len, fp);

    fclose(fp);

    struct sio_rtp_jpeg *jpeg = sio_rtp_jpeg_create(1420);

    while (1) {
        sio_rtp_jpeg_process(jpeg, jpegdata, len, sio_rtspmod_rtp_send);
        usleep(40 * 1000);
    }

    return NULL;
}

static inline
int sio_rtspmod_setup_response(sio_conn_t conn)
{
    sio_rtspmod_udpconn_create();

    struct sio_rtsp_conn *rtspconn = sio_rtspmod_get_rtspconn_from_conn(conn);
    sio_str_t *cseq = &rtspconn->cseq;
    sio_str_t *cliport = &rtspconn->cliport;

    char buffer[1024] = { 0 };
    sprintf(buffer, "RTSP/1.0 200 OK\r\n"
                    "CSeq: %.*s\r\n"
                    "Transport: %.*s;server_port=56400-56401\r\n"
                    "Session: 66334873\r\n"
                    "\r\n", cseq->length, cseq->data, cliport->length, cliport->data);

    sio_conn_write(conn, buffer, strlen(buffer));

    return 0;
}

static inline
int sio_rtspmod_play_response(sio_conn_t conn)
{
    struct sio_rtsp_conn *rtspconn = sio_rtspmod_get_rtspconn_from_conn(conn);
    sio_str_t *cseq = &rtspconn->cseq;

    char buffer[1024] = { 0 };
    sprintf(buffer, "RTSP/1.0 200 OK\r\n"
                    "CSeq: %.*s\r\n"
                    "Range: npt=0.000-\r\n"
                    "Session: 66334873; timeout=60\r\n"
                    "\r\n", cseq->length, cseq->data);

    sio_conn_write(conn, buffer, strlen(buffer));

    static struct sio_thread *thread = NULL;
    if (thread == NULL) {
        thread = sio_thread_create(sio_rtspmod_rtp_routine, NULL);
        sio_thread_start(thread);
    }

    return 0;
}

static inline
struct sio_rtsp_conn *sio_rtspmod_rtspconn_create(sio_conn_t conn)
{
    struct sio_rtsp_conn *rtspconn = malloc(sizeof(struct sio_rtsp_conn));
    SIO_COND_CHECK_RETURN_VAL(!rtspconn, NULL);

    memset(rtspconn, 0, sizeof(struct sio_rtsp_conn));

    char *mem = malloc(sizeof(char) * 40960);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!mem, NULL,
        free(rtspconn));

    memset(mem, 0, sizeof(char) * 40960);

    struct sio_rtsp_buffer *buf = &rtspconn->buf;
    buf->buf = mem;
    buf->pos = mem;
    buf->last = mem;
    buf->end = mem + sizeof(char) * 40960;

    struct sio_rtspprot *prot = sio_rtspprot_create();
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!prot, NULL,
        free(mem),
        free(rtspconn));
    
    union sio_rtspprot_opt opt = {
        .private = rtspconn
    };
    sio_rtspprot_setopt(prot, SIO_RTSPPROT_PRIVATE, &opt);
    opt.ops.prot_begin = sio_rtspmod_protobegin;
    opt.ops.prot_method = sio_rtspmod_protomethod;
    opt.ops.prot_url = sio_rtspmod_protourl;
    opt.ops.prot_filed = sio_rtspmod_protofiled;
    opt.ops.prot_value = sio_rtspmod_protovalue;
    opt.ops.prot_complete = sio_rtspmod_protocomplete;
    sio_rtspprot_setopt(prot, SIO_RTSPPROT_OPS, &opt);
    
    rtspconn->prot = prot;

    return rtspconn;
}

static inline
void sio_rtspmod_rtspconn_destory(struct sio_rtsp_conn *rtspconn)
{
    free(rtspconn->buf.buf);
    free(rtspconn);
}

int sio_rtspmod_create(void)
{
    return 0;
}

int sio_rtspmod_streamconn(sio_conn_t conn)
{
    struct sio_rtsp_conn *rtspconn = sio_rtspmod_rtspconn_create(conn);
    SIO_COND_CHECK_RETURN_VAL(!rtspconn, -1);

    // conn with rtspconn
    union sio_conn_opt opt = { .private = rtspconn };
    sio_conn_setopt(conn, SIO_CONN_PRIVATE, &opt);
    rtspconn->conn = conn;

    return 0;
}

int sio_rtspmod_streamin(sio_conn_t conn, const char *data, int len)
{
    struct sio_rtsp_conn *rtspconn = sio_rtspmod_get_rtspconn_from_conn(conn);
    struct sio_rtspprot *prot = rtspconn->prot;
    struct sio_rtsp_buffer *buf = &rtspconn->buf;

    int l = buf->end - buf->last;
    l = l > len ? len : l;
    memcpy(buf->last, data, l);
    buf->last += l;

    l = buf->last - buf->pos;
    l = sio_rtspprot_process(prot, buf->pos, l);

    if (rtspconn->complete != 1) {
        buf->pos += l;
        return 0;
    }

    rtspconn->complete = 0;
    buf->last = buf->pos = buf->buf; // reset

    int method = sio_rtspprot_method(prot);
    switch (method) {
    case 1 << SIO_RTSP_OPTIONS:
        sio_rtspmod_options_response(conn);
        break;
    
    case 1 << SIO_RTSP_DESCRIBE:
        sio_rtspmod_describe_response(conn);
        break;
    
    case 1 << SIO_RTSP_SETUP:
        sio_rtspmod_setup_response(conn);
        break;

    case 1 << SIO_RTSP_PLAY:
        sio_rtspmod_play_response(conn);
        break;
    
    default:
        break;
    }

    return 0;
}

int sio_rtspmod_streamclose(sio_conn_t conn)
{
    struct sio_rtsp_conn *rtspconn = sio_rtspmod_get_rtspconn_from_conn(conn);
    sio_rtspmod_rtspconn_destory(rtspconn);

    return 0;
}

int sio_rtspmod_destory(void)
{
    if (conn_rtp) {
        sio_conn_destory(conn_rtp);
        sio_conn_destory(conn_rtcp);
    }
    return 0;
}