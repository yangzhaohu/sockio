#include "sio_taskfifo.h"
#include <string.h>
#include <stdlib.h>
#include "sio_common.h"
#include "sio_thread.h"
#include "sio_fifo.h"
#include "sio_cond.h"
#include "sio_atomic.h"

struct sio_fifo_thread_pri
{
    unsigned short inx;
    struct sio_taskfifo *parent;
};

struct sio_fifo_thread
{
    struct sio_fifo_thread_pri pri;
    struct sio_thread *thread;
    struct sio_fifo *fio;

    sio_mutex mutex;
    sio_cond condin;
    sio_cond condout;
};

struct sio_taskfifo
{
    unsigned short pipe;
    struct sio_fifo_thread *ft;

    int loop;
    void *(*routine)(void *handle, void *data);
    void *arg;
};

static void *sio_fifo_thread_start_routine(void *arg)
{
    struct sio_fifo_thread *ft = (struct sio_fifo_thread *)arg;
    struct sio_fifo_thread_pri *pri = &ft->pri;
    struct sio_taskfifo *tf = pri->parent;
    unsigned int pipe = pri->inx;

    while (tf->loop) {
        void *data = NULL;
        int ret = sio_taskfifo_out(tf, pipe, &data, 1);
        if (ret) {
            tf->routine(tf->arg, data);
        }
    }

    return NULL;
}

static inline
int sio_fifos_destory(struct sio_taskfifo *tf)
{
    unsigned short pipe = tf->pipe;
    for (int i = 0; i < pipe; i++) {
        struct sio_fifo **fio = &tf->ft[i].fio;
        SIO_COND_CHECK_CALLOPS(*fio != NULL,
            sio_fifo_destory(*fio),
            *fio = NULL);
    }

    return 0;
}

static inline
int sio_fifos_create(struct sio_taskfifo *tf, unsigned int size)
{
    int ret = 0;
    unsigned short pipe = tf->pipe;
    for (int i = 0; i < pipe; i++) {
        struct sio_fifo **fio = &tf->ft[i].fio;
        *fio = sio_fifo_create(size, sizeof(void *));
        SIO_COND_CHECK_CALLOPS_BREAK(*fio == NULL,
            ret = -1);
    }

    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        sio_fifos_destory(tf));

    return 0;
}

static inline
int sio_threads_destory(struct sio_taskfifo *tf)
{
    tf->loop = 0;
    unsigned short pipe = tf->pipe;
    for (int i = 0; i < pipe; i++) {
        struct sio_fifo_thread *ft = &tf->ft[i];
        struct sio_thread **thread = &ft->thread;
        sio_mutex *mutex = &ft->mutex;
        sio_mutex_lock(mutex);
        sio_cond_wake(&ft->condin);
        sio_cond_wake(&ft->condout);
        sio_mutex_unlock(mutex);

        if (*thread != NULL) {
            sio_thread_destory(*thread);
            *thread = NULL;
        }
    }

    return 0;
}

static inline
int sio_threads_create(struct sio_taskfifo *tf)
{
    int ret = 0;
    unsigned short pipe = tf->pipe;

    for (int i = 0; i < pipe; i++) {
        struct sio_fifo_thread *ft = &tf->ft[i];
        struct sio_thread **thread = &ft->thread;

        ft->pri.inx = i;
        ft->pri.parent = tf;

        *thread = sio_thread_create(sio_fifo_thread_start_routine, ft);
        SIO_COND_CHECK_CALLOPS_BREAK(*thread == NULL,
            ret = -1);

        ret = sio_thread_start(*thread);
        SIO_COND_CHECK_BREAK(ret == -1);
    }

    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        sio_threads_destory(tf));

    return 0;
}

static inline
int sio_taskfifo_cond_init(struct sio_taskfifo *tf)
{
    unsigned short pipe = tf->pipe;

    for (int i = 0; i < pipe; i++) {
        struct sio_fifo_thread *ft = &tf->ft[i];
        sio_mutex_init(&ft->mutex);
        sio_cond_init(&ft->condin);
        sio_cond_init(&ft->condout);
    }

    return 0;
}

static inline
int sio_taskfifo_cond_uninit(struct sio_taskfifo *tf)
{
    unsigned short pipe = tf->pipe;

    for (int i = 0; i < pipe; i++) {
        struct sio_fifo_thread *ft = &tf->ft[i];
        sio_mutex_destory(&ft->mutex);
        sio_cond_destory(&ft->condin);
        sio_cond_destory(&ft->condout);
    }

    return 0;
}

struct sio_taskfifo *sio_taskfifo_create(unsigned short pipe, void *(*routine)(void *handle, void *data),
                                    void *arg, unsigned int size)
{
    struct sio_taskfifo *tf = malloc(sizeof(struct sio_taskfifo));
    SIO_COND_CHECK_RETURN_VAL(tf == NULL, NULL);

    memset(tf, 0, sizeof(struct sio_taskfifo));

    tf->ft = malloc(sizeof(struct sio_fifo_thread) * size);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(tf->ft == NULL, NULL,
        free(tf));

    memset(tf->ft, 0, sizeof(struct sio_fifo_thread) * size);
    tf->pipe = pipe;
    tf->routine = routine;
    tf->arg = arg;

    tf->loop = 1;

    sio_taskfifo_cond_init(tf);

    int ret = sio_fifos_create(tf, size);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, NULL,
        sio_taskfifo_cond_uninit(tf),
        free(tf->ft),
        free(tf));

    ret = sio_threads_create(tf);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, NULL,
        sio_taskfifo_cond_uninit(tf),
        sio_fifos_destory(tf),
        free(tf->ft),
        free(tf));

    return tf;
}

int sio_taskfifo_in(struct sio_taskfifo *tf, unsigned short pipe, void *entry, unsigned int size)
{
    SIO_COND_CHECK_RETURN_VAL(tf == NULL, -1);
    SIO_COND_CHECK_RETURN_VAL(pipe > tf->pipe - 1, -1);

    struct sio_fifo_thread *ft = &tf->ft[pipe];
    struct sio_fifo *fio = ft->fio;
    sio_mutex *mutex = &ft->mutex;

    sio_mutex_lock(mutex);
    int ret = sio_fifo_in(fio, entry, size);
    if (ret == 0) {
        sio_cond_wait(&ft->condin, mutex);
    }
    ret = sio_fifo_in(fio, entry, size);
    sio_mutex_unlock(mutex);

    sio_cond_wake(&ft->condout);

    return ret;
}

int sio_taskfifo_out(struct sio_taskfifo *tf, unsigned short pipe, void *entry, unsigned int size)
{
    SIO_COND_CHECK_RETURN_VAL(tf == NULL, -1);
    SIO_COND_CHECK_RETURN_VAL(pipe > tf->pipe - 1, -1);

    struct sio_fifo_thread *ft = &tf->ft[pipe];
    struct sio_fifo *fio = ft->fio;
    sio_mutex *mutex = &ft->mutex;

    sio_mutex_lock(mutex);
    int ret = sio_fifo_out(fio, entry, size);
    if (ret == 0) {
        sio_cond_wait(&ft->condout, mutex);
    }
    ret = sio_fifo_out(fio, entry, size);
    sio_cond_wake(&ft->condin);
    sio_mutex_unlock(mutex);

    return ret;
}

int sio_taskfifo_destory(struct sio_taskfifo *tf)
{
    SIO_COND_CHECK_RETURN_VAL(tf == NULL, -1);

    sio_threads_destory(tf);
    sio_fifos_destory(tf);
    sio_taskfifo_cond_uninit(tf);

    free(tf->ft);
    free(tf);

    return 0;
}