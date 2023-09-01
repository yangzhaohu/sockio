#include "sio_rtspipe.h"
#include <stdlib.h>
#include <string.h>
#include "sio_common.h"
#include "sio_socket.h"

struct sio_rtspipe_owner
{
    void *private;
    struct sio_rtspipe_ops ops;
};

#define SIO_RTSPIPE_BUTT SIO_RTSPIPE_AUDIO + 1
#define SIO_RTSPCHN_BUTT SIO_RTSPCHN_RTCP + 1

struct sio_rtspipe
{
    enum sio_rtspipe_over over;
    struct sio_socket *rtsp;
    struct sio_rtspipe_owner owner;

    struct sio_socket *sock[SIO_RTSPIPE_BUTT][SIO_RTSPCHN_BUTT];
    unsigned short chn[SIO_RTSPIPE_BUTT][SIO_RTSPCHN_BUTT];
};

static inline
int sio_rtspipe_udp(struct sio_socket **rtpsock, struct sio_socket **rtcpsock)
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
    
    *rtpsock = rtp;
    *rtcpsock = rtcp;
    
    port += 2;

    return port;
}

static inline
int sio_rtspipe_rtpack_readable_from(struct sio_socket *sock)
{
    char data[2048] = { 0 };
    struct sio_socket_addr peer = { 0 };
    int len = sio_socket_readfrom(sock, data, 2048, &peer);

    union sio_socket_opt opt = { 0 };
    sio_socket_getopt(sock, SIO_SOCK_PRIVATE, &opt);

    struct sio_rtspipe *rtpipe = opt.private;
    if (rtpipe->owner.ops.rtpack) {
        rtpipe->owner.ops.rtpack(rtpipe, data, len);
    }

    return 0;
}

static inline
int sio_rtspipe_rtpack_writeable_to(struct sio_socket *sock)
{
    return 0;
}

static inline
int sio_rtspipe_rtpack_writeable(struct sio_socket *sock)
{
    return 0;
}

struct sio_rtspipe *sio_rtspipe_create(struct sio_socket *rtsp, enum sio_rtspipe_over over)
{
    struct sio_rtspipe *rtpipe = malloc(sizeof(struct sio_rtspipe));
    SIO_COND_CHECK_RETURN_VAL(!rtpipe, NULL);

    memset(rtpipe, 0, sizeof(struct sio_rtspipe));

    rtpipe->over = over;
    rtpipe->rtsp = rtsp;

    return rtpipe;
}

int sio_rtspipe_open_videochn(struct sio_rtspipe *rtpipe, int rtp, int rtcp)
{
    SIO_COND_CHECK_RETURN_VAL(!rtpipe, -1);
    SIO_COND_CHECK_RETURN_VAL(rtpipe->sock[SIO_RTSPIPE_VIDEO][SIO_RTSPCHN_RTP] != NULL &&
        rtpipe->sock[SIO_RTSPIPE_VIDEO][SIO_RTSPCHN_RTCP] != NULL, 0);

    rtpipe->chn[SIO_RTSPIPE_VIDEO][SIO_RTSPCHN_RTP] = rtp;
    rtpipe->chn[SIO_RTSPIPE_VIDEO][SIO_RTSPCHN_RTCP] = rtcp;

    if (rtpipe->over == SIO_RTSPIPE_OVERTCP) {
        rtpipe->sock[SIO_RTSPIPE_VIDEO][SIO_RTSPCHN_RTP] = rtpipe->rtsp;
        rtpipe->sock[SIO_RTSPIPE_VIDEO][SIO_RTSPCHN_RTCP] = rtpipe->rtsp;
        return 0;
    }

    int ret = sio_rtspipe_udp(&rtpipe->sock[SIO_RTSPIPE_VIDEO][SIO_RTSPCHN_RTP],
        &rtpipe->sock[SIO_RTSPIPE_VIDEO][SIO_RTSPCHN_RTCP]);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        free(rtpipe));

    struct sio_socket *rtpsock = rtpipe->sock[SIO_RTSPIPE_VIDEO][SIO_RTSPCHN_RTP];
    // struct sio_socket *rtcpsock = rtpipe->sock[SIO_RTSPIPE_VIDEO][SIO_RTSPCHN_RTCP];

    union sio_socket_opt opt = {
        .ops.readfromable = sio_rtspipe_rtpack_readable_from,
        .ops.writetoable = sio_rtspipe_rtpack_writeable_to
    };
    sio_socket_setopt(rtpsock, SIO_SOCK_OPS, &opt);

    opt.private = rtpipe;
    sio_socket_setopt(rtpsock, SIO_SOCK_PRIVATE, &opt);

    // mplex
    sio_socket_getopt(rtpipe->rtsp, SIO_SOCK_MPLEX, &opt);
    sio_socket_setopt(rtpsock, SIO_SOCK_MPLEX, &opt);
    sio_socket_mplex(rtpsock, SIO_EV_OPT_ADD, SIO_EVENTS_IN);

    return 0;
}

int sio_rtspipe_open_audiochn(struct sio_rtspipe *rtpipe, int rtp, int rtcp)
{
    SIO_COND_CHECK_RETURN_VAL(!rtpipe, -1);
    SIO_COND_CHECK_RETURN_VAL(rtpipe->sock[SIO_RTSPIPE_AUDIO][SIO_RTSPCHN_RTP] != NULL &&
        rtpipe->sock[SIO_RTSPIPE_AUDIO][SIO_RTSPCHN_RTCP] != NULL, 0);

    rtpipe->chn[SIO_RTSPIPE_AUDIO][SIO_RTSPCHN_RTP] = rtp;
    rtpipe->chn[SIO_RTSPIPE_AUDIO][SIO_RTSPCHN_RTCP] = rtcp;

    if (rtpipe->over == SIO_RTSPIPE_OVERTCP) {
        rtpipe->sock[SIO_RTSPIPE_AUDIO][SIO_RTSPCHN_RTP] = rtpipe->rtsp;
        rtpipe->sock[SIO_RTSPIPE_AUDIO][SIO_RTSPCHN_RTCP] = rtpipe->rtsp;
        return 0;
    }

    int ret = sio_rtspipe_udp(&rtpipe->sock[SIO_RTSPIPE_AUDIO][SIO_RTSPCHN_RTP],
        &rtpipe->sock[SIO_RTSPIPE_AUDIO][SIO_RTSPCHN_RTCP]);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        free(rtpipe));

    return 0;
}

int sio_rtspipe_setopt(struct sio_rtspipe *rtpipe,
    enum sio_rtspipe_optcmd cmd, union sio_rtspipe_opt *opt)
{
    switch (cmd) {
    case SIO_RTPCHN_PRIVATE:
        rtpipe->owner.private = opt->private;
        break;

    case SIO_RTPCHN_OPS:
        rtpipe->owner.ops = opt->ops;
        break;

    default:
        break;
    }

    return 0;
}

int sio_rtspipe_getopt(struct sio_rtspipe *rtpipe,
    enum sio_rtspipe_optcmd cmd, union sio_rtspipe_opt *opt)
{
    switch (cmd)
    {
    case SIO_RTPCHN_PRIVATE:
        opt->private = rtpipe->owner.private;
        break;

    default:
        break;
    }

    return 0;
}

int sio_rtspipe_getchn(struct sio_rtspipe *rtpipe, enum sio_rtspipe_t pipe, enum sio_rtspchn_t chn)
{
    SIO_COND_CHECK_RETURN_VAL(!rtpipe, -1);
    SIO_COND_CHECK_RETURN_VAL(pipe < SIO_RTSPIPE_VIDEO || pipe >= SIO_RTSPIPE_BUTT ||
        chn < SIO_RTSPCHN_RTP || chn >= SIO_RTSPCHN_BUTT, -1);

    struct sio_socket_addr addr = { 0 };
    sio_socket_sockname(rtpipe->sock[pipe][chn], &addr);

    return addr.port;
}

int sio_rtspipe_rtpsend(struct sio_rtspipe *rtpipe, enum sio_rtspipe_t pipe, enum sio_rtspchn_t chn,
    const char *data, unsigned int len)
{
    SIO_COND_CHECK_RETURN_VAL(!rtpipe, -1);
    SIO_COND_CHECK_RETURN_VAL(pipe < SIO_RTSPIPE_VIDEO || pipe >= SIO_RTSPIPE_BUTT ||
        chn < SIO_RTSPCHN_RTP || chn >= SIO_RTSPCHN_BUTT, -1);

    struct sio_socket *sock = rtpipe->sock[pipe][chn];
    unsigned short chndst = rtpipe->chn[pipe][chn];
    int ret = 0;
    if (rtpipe->over == SIO_RTSPIPE_OVERTCP) {
        char tcphdr[4] = { 0 };
        tcphdr[0] = '$';
        tcphdr[1] = chndst & 0xFF;
        tcphdr[2] = (char)((len & 0xFF00) >> 8);
        tcphdr[3] = (char)(len & 0xFF);
        ret = sio_socket_write(sock, tcphdr, 4);
        ret = sio_socket_write(sock, data, len);
    } else {
        struct sio_socket_addr peer = {
            .addr = "127.0.0.1",
            .port = chndst
        };

        ret = sio_socket_writeto(sock, data, len, &peer);
    }

    return ret;
}

int sio_rtspipe_close(struct sio_rtspipe *rtpipe)
{
    SIO_COND_CHECK_RETURN_VAL(!rtpipe, -1);

    struct sio_socket *rtpsock = rtpipe->sock[SIO_RTSPIPE_VIDEO][SIO_RTSPCHN_RTP];
    struct sio_socket *rtcpsock = rtpipe->sock[SIO_RTSPIPE_VIDEO][SIO_RTSPCHN_RTCP];
    if (rtpsock != rtcpsock) {
        sio_socket_destory(rtpsock);
        sio_socket_destory(rtcpsock);
    }

    rtpsock = rtpipe->sock[SIO_RTSPIPE_AUDIO][SIO_RTSPCHN_RTP];
    rtcpsock = rtpipe->sock[SIO_RTSPIPE_AUDIO][SIO_RTSPCHN_RTCP];
    if (rtpsock != rtcpsock) {
        sio_socket_destory(rtpsock);
        sio_socket_destory(rtcpsock);
    }

    free(rtpipe);

    return 0;
}