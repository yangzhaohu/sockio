#include "sio_avdev.h"
#include <string.h>
#include "sio_common.h"
#include "sio_avdev_jpeg.h"

static struct sio_avdev g_avdev[] = {
    [SIO_AVDEV_JPEG] = {
        .open = sio_avdev_jpeg_open,
        .getinfo = sio_avdev_jpeg_getinfo,
        .readframe = sio_avdev_jpeg_readframe,
        .close = sio_avdev_jpeg_close
    }
};

struct sio_avdev *sio_avdev_find(enum sio_avdev_type type)
{
    SIO_COND_CHECK_RETURN_VAL(type < SIO_AVDEV_JPEG || type >= SIO_AVDEV_BUTT, NULL);

    struct sio_avdev *dev = &g_avdev[type];
    SIO_COND_CHECK_RETURN_VAL(!dev->open, NULL);

    return dev;
}