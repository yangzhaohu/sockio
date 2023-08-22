#include "sio_rtplive.h"
#include <stdlib.h>
#include <string.h>
#include "sio_common.h"

struct sio_rtplive
{
    struct sio_rtpchn *rtpchn;
};

struct sio_rtplive *sio_rtplive_create()
{
    struct sio_rtplive *rtplive = malloc(sizeof(struct sio_rtplive));
    SIO_COND_CHECK_RETURN_VAL(!rtplive, NULL);

    memset(rtplive, 0, sizeof(struct sio_rtplive));

    return rtplive;
}

int sio_rtplive_open(struct sio_rtplive *rtplive, const char *name)
{
    return 0;
}

int sio_rtplive_start(struct sio_rtplive *rtplive)
{
    return 0;
}

int sio_rtplive_attach_rtpchn(struct sio_rtplive *rtplive, struct sio_rtpchn *rtpchn)
{
    return 0;
}

int sio_rtplive_record(struct sio_rtplive *rtplive, const char *data, unsigned int len)
{
    return 0;
}

struct sio_rtplive *sio_rtplive_destory(struct sio_rtplive *rtplive)
{

}