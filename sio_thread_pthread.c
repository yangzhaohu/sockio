#include "sio_thread_pthread.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef LINUX
#include <pthread.h>
#endif
#include "sio_common.h"
#include "sio_thread_pri.h"
#include "sio_log.h"

#ifdef LINUX
sio_uptr_t sio_thread_pthread_create(struct sio_thread_pri *pri)
{
    pthread_t tid;
    if (pthread_create(&tid, NULL, pri->routine, pri->arg) != 0) {
        return -1;
    }

    return tid;
}

int sio_thread_pthread_join(sio_uptr_t tid)
{
    return pthread_join(tid, NULL);
}

int sio_thread_pthread_destory(sio_uptr_t tid)
{
    int ret = sio_thread_pthread_join(tid);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret != 0, -1,
        SIO_LOGE("pthread join err: %d", ret));

    return 0;
}

#endif