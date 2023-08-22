#include "sio_rtpchn.h"
#include <stdlib.h>
#include <string.h>
#include "sio_common.h"
#include "sio_socket.h"

enum sio_chn
{
    SIO_CHN_RTP,
    SIO_CHN_RTCP,
    SIO_CHN_BUTT
};

struct sio_rtpchn
{
    struct sio_socket *sock[SIO_CHN_BUTT];
    int rtpchn;
    int rtcpchn;
};

static inline
int sio_rtpchn_udp(struct sio_rtpchn *rtpchn)
{
    static unsigned short port = 40000;
    port = port >= 65532 ? 40000 : port;

    struct sio_socket *rtp = sio_socket_create(SIO_SOCK_UDP, NULL);
    SIO_COND_CHECK_RETURN_VAL(!rtp, -1);
    
    struct sio_socket *rtcp = sio_socket_create(SIO_SOCK_UDP, NULL);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!rtcp, -1,
        sio_socket_destory(rtp));

    struct sio_socket_addr addr = {"127.0.0.1", port};
    int ret = sio_socket_bind(rtp, &addr);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        sio_socket_destory(rtp),
        sio_socket_destory(rtcp));
    
    struct sio_socket_addr addr2 = {"127.0.0.1", port + 1};
    ret = sio_socket_bind(rtcp, &addr2);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        sio_socket_destory(rtp),
        sio_socket_destory(rtcp));
    
    rtpchn->sock[SIO_CHN_RTP] = rtp;
    rtpchn->sock[SIO_CHN_RTCP] = rtcp;
    
    port += 2;

    return port;
}

static inline
struct sio_rtpchn *sio_rtpchn_create()
{
    struct sio_rtpchn *rtpchn = malloc(sizeof(struct sio_rtpchn));
    SIO_COND_CHECK_RETURN_VAL(!rtpchn, NULL);

    memset(rtpchn, 0, sizeof(struct sio_rtpchn));

    return rtpchn;
}

static inline
int sio_rtpchn_rtpack_readable_from(struct sio_socket *sock)
{
    return 0;
}

static inline
int sio_rtpchn_rtpack_writeable_to(struct sio_socket *sock)
{
    return 0;
}

static inline
int sio_rtpchn_rtpack_writeable(struct sio_socket *sock)
{
    return 0;
}

struct sio_rtpchn *sio_rtpchn_overtcp_open(struct sio_socket *rtsp, int rtp, int rtcp)
{
    struct sio_rtpchn *rtpchn = sio_rtpchn_create();
    SIO_COND_CHECK_RETURN_VAL(!rtpchn, NULL);

    rtpchn->sock[SIO_CHN_RTP] = rtsp;
    rtpchn->sock[SIO_CHN_RTCP] = rtsp;

    rtpchn->rtpchn = rtp;
    rtpchn->rtcpchn = rtcp;

    union sio_socket_opt opt = { NULL };
    sio_socket_getopt(rtsp, SIO_SOCK_OPS, &opt);

    opt.ops.writeable = sio_rtpchn_rtpack_writeable;
    sio_socket_setopt(rtsp, SIO_SOCK_OPS, &opt);

    return rtpchn;
}

struct sio_rtpchn *sio_rtpchn_overudp_open(struct sio_socket *rtsp, int rtp, int rtcp)
{
    struct sio_rtpchn *rtpchn = sio_rtpchn_create();
    SIO_COND_CHECK_RETURN_VAL(!rtpchn, NULL);

    int ret = sio_rtpchn_udp(rtpchn);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, NULL,
        free(rtpchn));

    rtpchn->rtpchn = rtp;
    rtpchn->rtcpchn = rtcp;

    union sio_socket_opt opt = {
        .ops.readable = sio_rtpchn_rtpack_readable_from,
        .ops.writeable = sio_rtpchn_rtpack_writeable_to
    };
    sio_socket_setopt(rtpchn->sock[SIO_CHN_RTP], SIO_SOCK_OPS, &opt);

    return rtpchn;
}

int sio_rtpchn_chnrtp(struct sio_rtpchn *rtpchn)
{
    SIO_COND_CHECK_RETURN_VAL(!rtpchn, -1);

    struct sio_socket_addr addr = { 0 };
    sio_socket_sockname(rtpchn->sock[SIO_CHN_RTP], &addr);

    return addr.port;
}

int sio_rtpchn_chnrtcp(struct sio_rtpchn *rtpchn)
{
    SIO_COND_CHECK_RETURN_VAL(!rtpchn, -1);

    struct sio_socket_addr addr = { 0 };
    sio_socket_sockname(rtpchn->sock[SIO_CHN_RTCP], &addr);

    return addr.port;
}

int sio_rtpchn_rtpsend(struct sio_rtpchn *rtpchn, const char *data, unsigned int len)
{
    SIO_COND_CHECK_RETURN_VAL(!rtpchn, -1);

    struct sio_socket_addr peer = {
        .addr = "127.0.0.1",
        .port = rtpchn->rtpchn
    };

    return sio_socket_writeto(rtpchn->sock[SIO_CHN_RTP], data, len, &peer);
}

int sio_rtpchn_close(struct sio_rtpchn *rtpchn)
{
    SIO_COND_CHECK_RETURN_VAL(!rtpchn, -1);

    if (rtpchn->sock[SIO_CHN_RTP] != rtpchn->sock[SIO_CHN_RTCP]) {

    }

    free(rtpchn);

    return 0;
}