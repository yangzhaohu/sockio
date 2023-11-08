#ifndef SIO_THREAD_PRI_H_
#define SIO_THREAD_PRI_H_

#include "sio_def.h"

struct sio_thread_pri
{
    void *(*routine)(void *arg);
    void *arg;
};

struct sio_thread_ops
{
    sio_uptr_t (*create)(struct sio_thread_pri *pri);
    int (*self)(void);
    int (*join)(sio_uptr_t tid);
    int (*destory)(sio_uptr_t tid);
};

#endif