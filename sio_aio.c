#include "sio_event.h"
#include <stdlib.h>
#include <string.h>
#include "sio_common.h"
#include "sio_overlap.h"
#include "sio_aioseq.h"
#include "sio_log.h"

#define SIO_AIO_BUFF_PADDING 512

union sio_aioctx
{
#ifdef WIN32
    struct sio_overlap ctx;
#else
    struct sio_aioseq ctx;
#endif
};

static inline
void *sio_aiobuf_baseptr(sio_aiobuf buf)
{
    return (char *)buf - sizeof(union sio_aioctx);
}

static inline
int sio_aioctx_size_imp()
{
    return sizeof(union sio_aioctx);
}

sio_aiobuf sio_aiobuf_alloc(unsigned long size)
{
    size += SIO_AIO_BUFF_PADDING; // for ssl

    int must = sio_aioctx_size_imp();
    char *buf = malloc(size + must);

    return buf + must;
}

void *sio_aiobuf_aioctx_ptr(sio_aiobuf buf)
{
    return sio_aiobuf_baseptr(buf);
}

void sio_aiobuf_free(sio_aiobuf buf)
{
    buf = sio_aiobuf_baseptr(buf);
    free(buf);
}

int sio_aioctx_size()
{
    return sio_aioctx_size_imp();
}