#ifndef SIO_SERVMOD_H_
#define SIO_SERVMOD_H_

#include "sio_submod.h"

struct sio_servmod;

enum sio_servmod_optcmd
{
    SIO_SERVMOD_ADDR
};

struct sio_servmod_addr
{
    char addr[32];
    int port;
};

union sio_servmod_opt
{
    struct sio_servmod_addr addr;
};

#ifdef __cplusplus
extern "C" {
#endif

struct sio_servmod *sio_servmod_create(enum sio_submod_type type);

int sio_servmod_setopt(struct sio_servmod *servmod, enum sio_servmod_optcmd cmd, union sio_servmod_opt *opt);

int sio_servmod_getopt(struct sio_servmod *servmod, enum sio_servmod_optcmd cmd, union sio_servmod_opt *opt);

int sio_servmod_setlocat(struct sio_servmod *servmod, const struct sio_locate *locations, int size);

int sio_servmod_dowork(struct sio_servmod *servmod);

int sio_servmod_insmod(struct sio_servmod *servmod, const char *modname, struct sio_submod *mod);

int sio_servmod_destory(struct sio_servmod *servmod);

#ifdef __cplusplus
}
#endif

#endif