#include "sio_mplex_thread.h"
#include <string.h>
#include <stdlib.h>
#include "sio_common.h"
#include "sio_thread.h"
#include "sio_log.h"

enum sio_mplex_thread_state
{
    SIO_MPLEX_THREAD_DEFAULT,
    SIO_MPLEX_THREAD_RUNNING,
    SIO_MPLEX_THREAD_RUNSTOP,
    SIO_MPLEX_THREAD_STOPPED
};

struct sio_mplex_thread
{
    struct sio_mplex *mplex;
    struct sio_thread *thread;
    enum sio_mplex_thread_state tstate;
};

#define sio_mplex_thread_set_state(mpt, val)     mpt->tstate = val;
#define sio_mplex_thread_get_state(mpt)          mpt->tstate

extern int sio_socket_event_dispatch(struct sio_event *events, int count);

void *sio_mplex_thread_start_routine(void *arg)
{
    struct sio_mplex_thread *mpt = (struct sio_mplex_thread *)arg;
    struct sio_mplex *mplex = mpt->mplex;

    struct sio_event events[128];

    while (sio_mplex_thread_get_state(mpt) == SIO_MPLEX_THREAD_RUNNING) {
        int count = sio_mplex_wait(mplex, events, 128);
        if (count > 0) {
            sio_socket_event_dispatch(events, count);
        } else if (count == -1) {
            break;
        }
    }

    sio_mplex_thread_set_state(mpt, SIO_MPLEX_THREAD_STOPPED);

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
    sio_mplex_thread_set_state(mpt, SIO_MPLEX_THREAD_RUNNING);

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
    enum sio_mplex_thread_state state = sio_mplex_thread_get_state(mpt);
    while (state != SIO_MPLEX_THREAD_DEFAULT &&
        state != SIO_MPLEX_THREAD_STOPPED) {
            state = sio_mplex_thread_get_state(mpt);
        }
    return 0;
}

int sio_mplex_thread_destory(struct sio_mplex_thread *mpt)
{
    SIO_COND_CHECK_RETURN_VAL(!mpt, -1);

    sio_mplex_close(mpt->mplex);
    sio_mplex_thread_wait_thread_exit(mpt);

    sio_thread_destory(mpt->thread);
    sio_mplex_destory(mpt->mplex);

    free(mpt);
    
    return 0;
}