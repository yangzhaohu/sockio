#ifndef SIO_RTPVOD_H_
#define SIO_RTPVOD_H_

#include "sio_rtspdev.h"

struct sio_rtpvod;

#ifdef __cplusplus
extern "C" {
#endif

sio_rtspdev_t sio_rtpvod_open(const char *name);

int sio_rtpvod_play(sio_rtspdev_t dev);

int sio_rtpvod_get_describe(sio_rtspdev_t dev, const char **describe);

int sio_rtpvod_add_senddst(sio_rtspdev_t dev, struct sio_rtspipe *rtpchn);

int sio_rtpvod_close(sio_rtspdev_t dev);

#ifdef __cplusplus
}
#endif

#endif