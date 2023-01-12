#include "sio_mplex_thread.h"
#include <string.h>
#include <stdlib.h>
#include "sio_common.h"
#include "sio_thread.h"
#include "sio_log.h"

struct sio_mplex_thread
{
    struct sio_mplex *mplex;
    struct sio_thread *thread;
    int loop_flag;
    int external_mplex_flag;
};

#define sio_mplex_thread_external_mplex_set_flag(mpt, val)      mpt->external_mplex_flag = val;
#define sio_mplex_thread_external_mplex_flag(mpt)               mpt->external_mplex_flag

#define sio_mplex_thread_set_loop(mpt, val)     mpt->loop_flag = val;
#define sio_mplex_thread_loop(mpt)              mpt->loop_flag

extern int sio_socket_event_dispatch(struct sio_event *events, int count);

void *sio_mplex_thread_start_routine(void *arg)
{
    struct sio_mplex_thread *mpt = (struct sio_mplex_thread *)arg;
    struct sio_mplex *mplex = mpt->mplex;

    struct sio_event events[128];

    while (sio_mplex_thread_loop(mpt)) {
        int count = sio_mplex_wait(mplex, events, 128);
        if (count > 0) {
            sio_socket_event_dispatch(events, count);
        }
    }

    return NULL;
}

static inline
struct sio_mplex_thread *sio_mplex_thread_create_imp(struct sio_mplex *mplex)
{
    struct sio_mplex_thread *mpt = malloc(sizeof(struct sio_mplex_thread));
    SIO_COND_CHECK_RETURN_VAL(!mpt, NULL);

    memset(mpt, 0, sizeof(struct sio_mplex_thread));
    mpt->mplex = mplex;

    struct sio_thread *thread = sio_thread_create(sio_mplex_thread_start_routine, mpt);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!thread, NULL,
        free(mpt));

    mpt->thread = thread;
    sio_mplex_thread_set_loop(mpt, 1);

    int ret = sio_thread_start(thread);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, NULL,
        sio_thread_destory(thread),
        free(mpt));
    
    return mpt;
}

struct sio_mplex_thread *sio_mplex_thread_create(enum SIO_MPLEX_TYPE type)
{
    struct sio_mplex_attr attr = { .type = type };
    struct sio_mplex *mplex = sio_mplex_create(&attr);
    SIO_COND_CHECK_RETURN_VAL(!mplex, NULL);

    struct sio_mplex_thread *mpt = sio_mplex_thread_create_imp(mplex);
    SIO_COND_CHECK_RETURN_VAL(!mpt, NULL);

    sio_mplex_thread_external_mplex_set_flag(mpt, 0);
    
    return mpt;
}

struct sio_mplex_thread *sio_mplex_thread_create2(struct sio_mplex *mplex)
{
    SIO_COND_CHECK_RETURN_VAL(!mplex, NULL);

    struct sio_mplex_thread *mpt = sio_mplex_thread_create_imp(mplex);
    SIO_COND_CHECK_RETURN_VAL(!mpt, NULL);

    sio_mplex_thread_external_mplex_set_flag(mpt, 1);
    
    return mpt;
}

struct sio_mplex *sio_mplex_thread_mplex_ref(struct sio_mplex_thread *mpt)
{
    SIO_COND_CHECK_RETURN_VAL(!mpt, NULL);

    return mpt->mplex;
}

static inline
int sio_mplex_thread_wait_thread_exit(struct sio_mplex_thread *mpt)
{
    sio_mplex_thread_set_loop(mpt, 0);
    return 0;
}

int sio_mplex_thread_destory(struct sio_mplex_thread *mpt)
{
    SIO_COND_CHECK_RETURN_VAL(!mpt, -1);

    sio_mplex_thread_wait_thread_exit(mpt);
    sio_thread_destory(mpt->thread);
    if (sio_mplex_thread_external_mplex_flag(mpt) == 0) {
        sio_mplex_close(mpt->mplex);
    }
    free(mpt);
    
    return 0;
}