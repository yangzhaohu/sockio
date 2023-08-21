#ifndef SIO_AVDEV_H_
#define SIO_AVDEV_H_

#include "sio_avframe.h"

enum sio_avdev_type
{
    SIO_AVDEV_JPEG,
    SIO_AVDEV_VIDEO,
    SIO_AVDEV_CAM,
    SIO_AVDEV_BUTT
};

typedef void * sio_avdev_t;

struct sio_avdev
{
    sio_avdev_t handle;
    sio_avdev_t (*open)(const char *name);
    int (*getinfo)(sio_avdev_t dev, struct sio_avinfo *info);
    int (*readframe)(sio_avdev_t dev, struct sio_avframe *frame);
    int (*close)(sio_avdev_t dev);
};

#ifdef __cplusplus
extern "C" {
#endif

struct sio_avdev *sio_avdev_find(enum sio_avdev_type type);

#ifdef __cplusplus
}
#endif


#endif