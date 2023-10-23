#ifndef SIO_MPLEX_THREAD_H_
#define SIO_MPLEX_THREAD_H_

#include "sio_mplex.h"

struct sio_permplex;

#ifdef __cplusplus
extern "C" {
#endif

struct sio_permplex *sio_permplex_create(enum SIO_MPLEX_TYPE type);

struct sio_mplex *sio_permplex_mplex_ref(struct sio_permplex *pmplex);

int sio_permplex_destory(struct sio_permplex *pmplex);

#ifdef __cplusplus
}
#endif

#endif