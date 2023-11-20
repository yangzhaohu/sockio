#include "sio_permplex.h"
#include <string.h>
#include <stdlib.h>
#include "sio_common.h"
#include "sio_thread.h"
#include "sio_log.h"

struct sio_permplex
{
    struct sio_mplex *mplex;
    struct sio_thread *thread;
};


extern int sio_socket_event_dispatch(struct sio_event *events, int count);

void *sio_permplex_start_routine(void *arg)
{
    struct sio_permplex *pmplex = (struct sio_permplex *)arg;
    struct sio_mplex *mplex = pmplex->mplex;

    while (1) {
        struct sio_event events[128] = { 0 };
        int count = sio_mplex_wait(mplex, events, 128);
        if (count > 0) {
            sio_socket_event_dispatch(events, count);
        } else if (count == -1) {
            break;
        }
    }

    return NULL;
}

static inline
struct sio_permplex *sio_permplex_create_imp(struct sio_mplex *mplex)
{
    struct sio_permplex *pmplex = malloc(sizeof(struct sio_permplex));
    SIO_COND_CHECK_RETURN_VAL(!pmplex, NULL);

    memset(pmplex, 0, sizeof(struct sio_permplex));
    pmplex->mplex = mplex;

    struct sio_thread *thread = sio_thread_create(sio_permplex_start_routine, pmplex);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!thread, NULL,
        free(pmplex));

    pmplex->thread = thread;

    int ret = sio_thread_start(thread);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, NULL,
        sio_thread_destory(thread),
        free(pmplex));
    
    return pmplex;
}

struct sio_permplex *sio_permplex_create(enum SIO_MPLEX_TYPE type)
{
    struct sio_mplex_attr attr = { .type = type };
    struct sio_mplex *mplex = sio_mplex_create(&attr);
    SIO_COND_CHECK_RETURN_VAL(!mplex, NULL);

    struct sio_permplex *pmplex = sio_permplex_create_imp(mplex);
    SIO_COND_CHECK_RETURN_VAL(!pmplex, NULL);
    
    return pmplex;
}

struct sio_mplex *sio_permplex_mplex_ref(struct sio_permplex *pmplex)
{
    SIO_COND_CHECK_RETURN_VAL(!pmplex, NULL);

    return pmplex->mplex;
}

int sio_permplex_destory(struct sio_permplex *pmplex)
{
    SIO_COND_CHECK_RETURN_VAL(!pmplex, -1);

    sio_mplex_close(pmplex->mplex);

    sio_thread_destory(pmplex->thread);
    sio_mplex_destory(pmplex->mplex);

    free(pmplex);
    
    return 0;
}