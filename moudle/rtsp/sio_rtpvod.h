#ifndef SIO_RTPVOD_H_
#define SIO_RTPVOD_H_

struct sio_rtpvod;
struct sio_rtpchn;
struct sio_rtpstream;
struct sio_socket;

#include "sio_rtpchn.h"
#include "sio_rtpstream.h"

#ifdef __cplusplus
extern "C" {
#endif

struct sio_rtpvod *sio_rtpvod_create();

struct sio_rtpchn *sio_rtpvod_open_rtpchn(struct sio_rtpvod *rtpvod, struct sio_socket *rtsp,
    int rtp, int rtcp, enum sio_rtpchn_over over);

struct sio_rtpstream *sio_rtpvod_open_rtpstream(struct sio_rtpvod *rtpvod, const char *name);

struct sio_rtpvod *sio_rtpvod_destory(struct sio_rtpvod *rtpvod);

#ifdef __cplusplus
}
#endif

#endif