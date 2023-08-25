#include "sio_rtpvod.h"
#include <stdlib.h>
#include <string.h>
#include "sio_common.h"
#include "sio_rtpchn.h"
#include "sio_rtpstream.h"
#include "sio_log.h"

struct sio_rtpvod
{
    struct sio_rtpchn *rtpchn;
    struct sio_rtpstream *stream;
};

const char *g_describe = "v=0\r\n"
                         "o=- 91565340853 1 in IP4 127.0.0.1\r\n"
                         "t=0 0\r\n"
                         "a=contol:*\r\n"
                         "m=video 0 RTP/AVP 26\r\n"
                         "a=rtpmap:26 JPEG/90000\r\n"
                         "a=control:track0\r\n";

static inline
void sio_rtpvod_rtpack_send(struct sio_rtpstream *stream, struct sio_packet *packet)
{
    union sio_rtpstream_opt opt = { 0 };
    sio_rtpstream_getopt(stream, SIO_RTPSTREAM_PRIVATE, &opt);

    struct sio_rtpvod *rtpvod = opt.private;
    struct sio_rtpchn *rtpchn = rtpvod->rtpchn;

    sio_rtpchn_rtpsend(rtpchn, (char *)packet->data, packet->length);
}

static inline
struct sio_rtpvod *sio_rtpvod_create()
{
    struct sio_rtpvod *rtpvod = malloc(sizeof(struct sio_rtpvod));
    SIO_COND_CHECK_RETURN_VAL(!rtpvod, NULL);

    memset(rtpvod, 0, sizeof(struct sio_rtpvod));

    return rtpvod;
}

int sio_rtpvod_add_senddst(sio_rtspdev_t dev, struct sio_rtpchn *rtpchn)
{
    struct sio_rtpvod *rtpvod = (struct sio_rtpvod *)dev;

    rtpvod->rtpchn = rtpchn;

    return 0;
}

sio_rtspdev_t sio_rtpvod_open(const char *name)
{
    struct sio_rtpvod *rtpvod = sio_rtpvod_create();
    SIO_COND_CHECK_RETURN_VAL(!rtpvod, NULL);

    struct sio_rtpstream *stream = sio_rtpstream_open(name);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!stream, NULL,
        free(rtpvod));

    union sio_rtpstream_opt opt = { .private = rtpvod };
    sio_rtpstream_setopt(stream, SIO_RTPSTREAM_PRIVATE, &opt);
    opt.ops.rtpack = sio_rtpvod_rtpack_send;
    sio_rtpstream_setopt(stream, SIO_RTPSTREAM_OPS, &opt);

    rtpvod->stream = stream;

    return (sio_rtspdev_t)rtpvod;
}

int sio_rtpvod_play(sio_rtspdev_t dev)
{
    struct sio_rtpvod *rtpvod = (struct sio_rtpvod *)dev;

    return sio_rtpstream_start(rtpvod->stream);
}

int sio_rtpvod_get_describe(sio_rtspdev_t dev, const char **describe)
{
    *describe = g_describe;
    return 0;
}

int sio_rtpvod_close(sio_rtspdev_t dev)
{
    struct sio_rtpvod *rtpvod = (struct sio_rtpvod *)dev;

    SIO_COND_CHECK_CALLOPS(rtpvod->stream,
        sio_rtpstream_close(rtpvod->stream));

    return 0;
}