#include "sio_httpmod.h"
#include <string.h>
#include <stdlib.h>
#include "sio_common.h"
#include "sio_log.h"

struct sio_httpmod
{
    int a;
};


int sio_httpmod_type()
{
    return 1;
}
int sio_httpmod_version(const char **version)
{
    *version = "0.0.1";
    return 0;
}

int sio_httpmod_create(void **mod)
{
    struct sio_httpmod *httpmod = malloc(sizeof(struct sio_httpmod));
    *mod = httpmod;

    return 0;
}

int sio_httpmod_streamin(void *mod, const char *data, int len)
{
    SIO_LOGE("%s\n", data);
    return 0;
}

int sio_httpmod_destory(void *mod)
{
    free(mod);

    return 0;
}