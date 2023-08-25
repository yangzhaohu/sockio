#ifndef SIO_RTPLIVE_H_
#define SIO_RTPLIVE_H_

#include "sio_rtspdev.h"

struct sio_rtplive;

#ifdef __cplusplus
extern "C" {
#endif

sio_rtspdev_t sio_rtplive_open(const char *name);

int sio_rtplive_set_describe(sio_rtspdev_t dev, const char *describe, int len);

int sio_rtplive_get_describe(sio_rtspdev_t dev, const char **describe);

int sio_rtplive_add_senddst(sio_rtspdev_t dev, struct sio_rtpchn *rtpchn);

int sio_rtplive_rm_senddst(sio_rtspdev_t dev, struct sio_rtpchn *rtpchn);

int sio_rtplive_record(sio_rtspdev_t dev, const char *data, unsigned int len);

int sio_rtplive_close(sio_rtspdev_t dev);

#ifdef __cplusplus
}
#endif

#endif