#ifndef SIO_SSLCTX_H_
#define SIO_SSLCTX_H_

#include "sio_sslpri.h"

typedef void * sio_sslctx_t;

#ifdef __cplusplus
extern "C" {
#endif

sio_sslctx_t sio_sslctx_create();

int sio_sslctx_setopt(sio_sslctx_t ctx, enum sio_sslopc cmd, union sio_sslopt *opt);

int sio_sslctx_destory(sio_sslctx_t ctx);

#ifdef __cplusplus
}
#endif

#endif