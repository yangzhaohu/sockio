#ifndef SIO_RTPVOD_H_
#define SIO_RTPVOD_H_

#include "sio_rtpchn.h"
#include "sio_rtpstream.h"

struct sio_rtpvod;
struct sio_socket;

#ifdef __cplusplus
extern "C" {
#endif

struct sio_rtpvod *sio_rtpvod_create();

int sio_rtpvod_open(struct sio_rtpvod *rtpvod, const char *name);

int sio_rtpvod_start(struct sio_rtpvod *rtpvod);

int sio_rtpvod_attach_rtpchn(struct sio_rtpvod *rtpvod, struct sio_rtpchn *rtpchn);

struct sio_rtpvod *sio_rtpvod_destory(struct sio_rtpvod *rtpvod);

#ifdef __cplusplus
}
#endif

#endif