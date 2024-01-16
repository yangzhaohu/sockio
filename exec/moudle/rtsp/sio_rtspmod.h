#ifndef SIO_RTSPMOD_H_
#define SIO_RTSPMOD_H_

#include "sio_submod.h"

struct sio_location;

#ifdef __cplusplus
extern "C" {
#endif

int sio_rtspmod_name(const char **name);
int sio_rtspmod_type(void);
int sio_rtspmod_version(const char **version);

sio_submod_t sio_rtspmod_create(void);

int sio_rtspmod_setlocate(sio_submod_t mod, const struct sio_location *locations, int size);

int sio_rtspmod_newconn(sio_submod_t mod, struct sio_server *server);

int sio_rtspmod_destory(sio_submod_t mod);

#ifdef __cplusplus
}
#endif

#endif