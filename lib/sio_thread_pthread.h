#ifndef SIO_THREAD_PTHREAD_H_
#define SIO_THREAD_PTHREAD_H_

#include "sio_thread_pri.h"

#ifdef __cplusplus
extern "C" {
#endif

sio_uptr_t sio_thread_pthread_create(struct sio_thread_pri *pri);

int sio_thread_pthread_join(sio_uptr_t tid);

int sio_thread_pthread_destory(sio_uptr_t tid);

#ifdef __cplusplus
}
#endif

#endif