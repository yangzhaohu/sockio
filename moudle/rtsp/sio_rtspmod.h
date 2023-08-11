#ifndef SIO_RTSPMOD_H_
#define SIO_RTSPMOD_H_

#include "sio_submod.h"

#ifdef __cplusplus
extern "C" {
#endif

int sio_rtspmod_name(const char **name);
int sio_rtspmod_type();
int sio_rtspmod_version(const char **version);

int sio_rtspmod_create(void);

int sio_rtspmod_newconn(struct sio_server *server);

int sio_rtspmod_destory(void);

#ifdef __cplusplus
}
#endif

#endif