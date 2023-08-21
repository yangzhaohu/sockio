#include "sio_rtpvod.h"
#include <stdlib.h>
#include <string.h>
#include "sio_common.h"
#include "sio_log.h"

struct sio_rtpvod
{
    struct sio_rtpchn *rtpchn;
    struct sio_rtpstream *stream;
};

static inline
void sio_rtpvod_rtpack_send(struct sio_rtpstream *stream, struct sio_packet *packet)
{
    union sio_rtpstream_opt opt = { 0 };
    sio_rtpstream_getopt(stream, SIO_RTPSTREAM_PRIVATE, &opt);

    struct sio_rtpvod *rtpvod = opt.private;
    struct sio_rtpchn *rtpchn = rtpvod->rtpchn;

    sio_rtpchn_rtpsend(rtpchn, (char *)packet->data, packet->length);
}

struct sio_rtpvod *sio_rtpvod_create()
{
    struct sio_rtpvod *rtpvod = malloc(sizeof(struct sio_rtpvod));
    SIO_COND_CHECK_RETURN_VAL(!rtpvod, NULL);

    memset(rtpvod, 0, sizeof(struct sio_rtpvod));

    return rtpvod;
}

struct sio_rtpchn *sio_rtpvod_open_rtpchn(struct sio_rtpvod *rtpvod, struct sio_socket *rtsp,
    int rtp, int rtcp, enum sio_rtpchn_over over)
{
    SIO_COND_CHECK_RETURN_VAL(!rtpvod || !rtsp, NULL);
    SIO_COND_CHECK_RETURN_VAL(rtpvod->rtpchn, 0);

    if (over == SIO_RTPCHN_OVER_UDP) {
        rtpvod->rtpchn = sio_rtpchn_overudp_open(rtsp, rtp, rtcp);
    } else {
        rtpvod->rtpchn = sio_rtpchn_overtcp_open(rtsp, rtp, rtcp);
    }

    SIO_COND_CHECK_RETURN_VAL(!rtpvod->rtpchn, NULL);

    struct sio_rtpstream *stream = rtpvod->stream;
    union sio_rtpstream_opt opt = { .private = rtpvod };
    sio_rtpstream_setopt(stream, SIO_RTPSTREAM_PRIVATE, &opt);
    opt.ops.rtpack = sio_rtpvod_rtpack_send;
    sio_rtpstream_setopt(stream, SIO_RTPSTREAM_OPS, &opt);

    return rtpvod->rtpchn;
}

struct sio_rtpstream *sio_rtpvod_open_rtpstream(struct sio_rtpvod *rtpvod, const char *name)
{
    SIO_COND_CHECK_RETURN_VAL(!rtpvod || !name, NULL);
    SIO_COND_CHECK_RETURN_VAL(rtpvod->stream, 0);

    rtpvod->stream = sio_rtpstream_open(name);

    SIO_COND_CHECK_RETURN_VAL(!rtpvod->stream, NULL);

    return rtpvod->stream;
}

struct sio_rtpvod *sio_rtpvod_destory(struct sio_rtpvod *rtpvod)
{
    SIO_COND_CHECK_RETURN_VAL(!rtpvod, NULL);

    SIO_COND_CHECK_CALLOPS(rtpvod->rtpchn,
        sio_rtpchn_close(rtpvod->rtpchn));
    
    SIO_COND_CHECK_CALLOPS(rtpvod->stream,
        sio_rtpstream_close(rtpvod->stream));

    return 0;
}