#ifndef SIO_AVDEV_JPEG_H_
#define SIO_AVDEV_JPEG_H_

#include "sio_avdev.h"

#ifdef __cplusplus
extern "C" {
#endif

sio_avdev_t sio_avdev_jpeg_open(const char *name);

int sio_avdev_jpeg_getinfo(sio_avdev_t avdev, struct sio_avinfo *info);

int sio_avdev_jpeg_readframe(sio_avdev_t avdev, struct sio_avframe *frame);

int sio_avdev_jpeg_close(sio_avdev_t avdev);

#ifdef __cplusplus
}
#endif

#endif