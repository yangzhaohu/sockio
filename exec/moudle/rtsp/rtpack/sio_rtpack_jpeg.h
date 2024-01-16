#ifndef SIO_RTP_JPEG_H_
#define SIO_RTP_JPEG_H_

#include "sio_rtpack.h"

struct sio_rtpack_jpeg;

#ifdef __cplusplus
extern "C" {
#endif

sio_rtpack_t sio_rtpack_jpeg_create(unsigned int size);

int sio_rtpack_jpeg_packet(sio_rtpack_t rtpack, struct sio_avframe *frame, 
    void *handle, void (*package)(void *handle, struct sio_packet *pack));

int sio_rtpack_jpeg_destory(sio_rtpack_t rtpack);

#ifdef __cplusplus
}
#endif

#endif