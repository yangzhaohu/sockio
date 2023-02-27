#ifndef SIO_MPLEX_THREAD_H_
#define SIO_MPLEX_THREAD_H_

#include "sio_mplex.h"

struct sio_mplex_thread;

#ifdef __cplusplus
extern "C" {
#endif

struct sio_mplex_thread *sio_mplex_thread_create(enum SIO_MPLEX_TYPE type);

struct sio_mplex *sio_mplex_thread_mplex_ref(struct sio_mplex_thread *mpt);

int sio_mplex_thread_destory(struct sio_mplex_thread *mpt);

#ifdef __cplusplus
}
#endif

#endif