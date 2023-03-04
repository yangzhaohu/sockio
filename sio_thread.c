#include "sio_thread.h"
#include <string.h>
#include <stdlib.h>
#include "sio_common.h"
#include "sio_thread_pri.h"
#include "sio_thread_pthread.h"
#include "sio_thread_win.h"
#include "sio_log.h"

struct sio_thread
{
    unsigned long int tid;
    const struct sio_thread_ops *ops;
    struct sio_thread_pri pri;
};

static const
struct sio_thread_ops g_phtread_ops =
#ifdef WIN32
{
    .create = sio_thread_win_create,
    .destory = sio_thread_win_destory
};
#else
{
    .create = sio_thread_pthread_create,
    .destory = sio_thread_pthread_destory
};
#endif 

struct sio_thread *sio_thread_create(void *(*routine)(void *arg), void *arg)
{
    SIO_COND_CHECK_RETURN_VAL(!routine, NULL);

    struct sio_thread *thread = malloc(sizeof(struct sio_thread));
    SIO_COND_CHECK_RETURN_VAL(!thread, NULL);

    memset(thread, 0, sizeof(struct sio_thread));

    thread->ops = &g_phtread_ops;

    struct sio_thread_pri *pri = &thread->pri;
    pri->routine = routine;
    pri->arg = arg;

    return thread;
}

int sio_thread_start(struct sio_thread *thread)
{
    SIO_COND_CHECK_RETURN_VAL(!thread, -1);

    struct sio_thread_pri *pri = &thread->pri;
    const struct sio_thread_ops *ops = thread->ops;

    unsigned long int tid = ops->create(pri);
    SIO_COND_CHECK_RETURN_VAL(tid == -1, -1);

    thread->tid = tid;

    return 0;
}

// int sio_thread_stop(struct sio_thread *thread)
// {
//     return 0;
// }

int sio_thread_destory(struct sio_thread *thread)
{
    SIO_COND_CHECK_RETURN_VAL(!thread, -1);

    const struct sio_thread_ops *ops = thread->ops;

    unsigned long int tid = thread->tid;
    int ret = ops->destory(tid);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    free(thread);

    return ret;
}