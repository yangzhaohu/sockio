#ifndef SIO_RTPACK_H_
#define SIO_RTPACK_H_

#include "sio_avframe.h"

enum sio_rtpack_type
{
    SIO_RTPACK_JPEG,
    SIO_RTPACK_H264,
    SIO_RTPACK_H265,
    SIO_RTPACK_BUTT
};

typedef void * sio_rtpack_t;

struct sio_packet
{
    unsigned char *data;
    unsigned int length;
};

struct sio_rtpack
{
    sio_rtpack_t handle;
    sio_rtpack_t (*open)(unsigned int size);
    int (*packet)(sio_rtpack_t pack, struct sio_avframe *frame, 
        void *handle, void (*package)(void *handle, struct sio_packet *pack));
    int (*close)(sio_rtpack_t pack);
};

#ifdef __cplusplus
extern "C" {
#endif

struct sio_rtpack *sio_rtpack_find(enum sio_rtpack_type type);

#ifdef __cplusplus
}
#endif

#endif