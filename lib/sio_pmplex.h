#ifndef SIO_PMPLEX_H_
#define SIO_PMPLEX_H_

#include "sio_mplex.h"

struct sio_pmplex;

#ifdef __cplusplus
extern "C" {
#endif

struct sio_pmplex *sio_pmplex_create(enum SIO_MPLEX_TYPE type);

struct sio_mplex *sio_pmplex_mplex_ref(struct sio_pmplex *pmplex);

int sio_pmplex_destory(struct sio_pmplex *pmplex);

#ifdef __cplusplus
}
#endif

#endif