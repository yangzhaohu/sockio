#ifndef SIO_HTTPMOD_H_
#define SIO_HTTPMOD_H_

#include "sio_submod.h"

struct sio_submod;
struct sio_location;

#ifdef __cplusplus
extern "C" {
#endif

int sio_httpmod_name(const char **name);
int sio_httpmod_type();
int sio_httpmod_version(const char **version);

sio_submod_t sio_httpmod_create(void);

int sio_httpmod_setlocate(sio_submod_t mod, const struct sio_location *locations, int size);

int sio_httpmod_modhook(sio_submod_t mod, struct sio_submod *submod);

int sio_httpmod_newconn(sio_submod_t mod, struct sio_server *server);

int sio_httpmod_destory(sio_submod_t mod);

#ifdef __cplusplus
}
#endif

#endif