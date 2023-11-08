#ifndef SIO_THREAD_WIN_H_
#define SIO_THREAD_WIN_H_

#include "sio_thread_pri.h"

#ifdef __cplusplus
extern "C" {
#endif

sio_uptr_t sio_thread_win_create(struct sio_thread_pri *pri);

int sio_thread_win_join(sio_uptr_t tid);

int sio_thread_win_destory(sio_uptr_t tid);

#ifdef __cplusplus
}
#endif

#endif