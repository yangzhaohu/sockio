#ifndef SIO_DOIPMOD_H_
#define SIO_DOIPMOD_H_

#include "sio_submod.h"

#ifdef __cplusplus
extern "C" {
#endif

int sio_doipmod_name(const char **name);
int sio_doipmod_type(void);
int sio_doipmod_version(const char **version);

sio_submod_t sio_doipmod_create(void);

int sio_doipmod_newconn(sio_submod_t mod, struct sio_server *server);

int sio_doipmod_setlocate(sio_submod_t mod, const struct sio_location *locations, int size);

int sio_doipmod_destory(sio_submod_t mod);

#ifdef __cplusplus
}
#endif


#endif