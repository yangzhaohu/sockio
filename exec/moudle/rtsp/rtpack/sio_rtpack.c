#include "sio_rtpack.h"
#include <string.h>
#include "sio_common.h"
#include "sio_rtpack_jpeg.h"

static struct sio_rtpack g_rtpack[] = {
    [SIO_RTPACK_JPEG] = {
        .open = sio_rtpack_jpeg_create,
        .packet = sio_rtpack_jpeg_packet,
        .close = sio_rtpack_jpeg_destory
    }
};

struct sio_rtpack *sio_rtpack_find(enum sio_rtpack_type type)
{
    SIO_COND_CHECK_RETURN_VAL(type < SIO_RTPACK_JPEG || type >= SIO_RTPACK_BUTT, NULL);

    struct sio_rtpack *pack = &g_rtpack[type];
    SIO_COND_CHECK_RETURN_VAL(!pack->open, NULL);

    return pack;
}