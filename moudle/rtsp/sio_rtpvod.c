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

int sio_rtpvod_attach_rtpchn(struct sio_rtpvod *rtpvod, struct sio_rtpchn *rtpchn)
{
    SIO_COND_CHECK_RETURN_VAL(!rtpvod, -1);
    SIO_COND_CHECK_RETURN_VAL(rtpvod->rtpchn, -1);

    rtpvod->rtpchn = rtpchn;

    return 0;
}

int sio_rtpvod_open(struct sio_rtpvod *rtpvod, const char *name)
{
    SIO_COND_CHECK_RETURN_VAL(!rtpvod || !name, -1);
    SIO_COND_CHECK_RETURN_VAL(rtpvod->stream, 0);

    struct sio_rtpstream *stream = sio_rtpstream_open(name);
    SIO_COND_CHECK_RETURN_VAL(!stream, -1);

    union sio_rtpstream_opt opt = { .private = rtpvod };
    sio_rtpstream_setopt(stream, SIO_RTPSTREAM_PRIVATE, &opt);
    opt.ops.rtpack = sio_rtpvod_rtpack_send;
    sio_rtpstream_setopt(stream, SIO_RTPSTREAM_OPS, &opt);

    rtpvod->stream = stream;

    return 0;
}

int sio_rtpvod_start(struct sio_rtpvod *rtpvod)
{
    SIO_COND_CHECK_RETURN_VAL(!rtpvod, -1);

    return sio_rtpstream_start(rtpvod->stream);
}

struct sio_rtpvod *sio_rtpvod_destory(struct sio_rtpvod *rtpvod)
{
    SIO_COND_CHECK_RETURN_VAL(!rtpvod, NULL);

    // SIO_COND_CHECK_CALLOPS(rtpvod->rtpchn,
    //     sio_rtpchn_close(rtpvod->rtpchn));
    
    SIO_COND_CHECK_CALLOPS(rtpvod->stream,
        sio_rtpstream_close(rtpvod->stream));

    return 0;
}