#include "sio_timer.h"
#include <stdlib.h>
#include <string.h>
#include "sio_common.h"
#include "sio_timer_posix.h"
#include "sio_timer_win.h"
#include "sio_log.h"

struct sio_timer
{
    sio_timer_t timerid;
    const struct sio_timer_ops *ops;
    struct sio_timer_pri pri;
};

static const
struct sio_timer_ops g_timer_ops =
#ifdef WIN32
{
    .create = sio_timer_win_create,
    .start = sio_timer_win_start,
    .stop = sio_timer_win_stop,
    .destory = sio_timer_win_destory
};
#else
{
    .create = sio_timer_posix_create,
    .start = sio_timer_posix_start,
    .stop = sio_timer_posix_stop,
    .destory = sio_timer_posix_destory
};
#endif 

struct sio_timer *sio_timer_create(void *(*routine)(void *arg), void *arg)
{
    struct sio_timer *timer = malloc(sizeof(struct sio_timer));
    SIO_COND_CHECK_RETURN_VAL(!timer, NULL);

    memset(timer, 0, sizeof(struct sio_timer));

    struct sio_timer_pri *pri = &timer->pri;
    pri->routine = routine;
    pri->arg = arg;

    timer->ops = &g_timer_ops;
    sio_timer_t timerid = timer->ops->create(pri);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(timerid == NULL, NULL,
        free(timer));

    timer->timerid = timerid;

    return timer;
}

int sio_timer_start(struct sio_timer *timer, unsigned long int udelay, unsigned long int uperiod)
{
    SIO_COND_CHECK_RETURN_VAL(!timer, -1);

    const struct sio_timer_ops *ops = timer->ops;
    return  ops->start(timer->timerid, udelay, uperiod);
}

int sio_timer_stop(struct sio_timer *timer)
{
    SIO_COND_CHECK_RETURN_VAL(!timer, -1);

    const struct sio_timer_ops *ops = timer->ops;
    return  ops->stop(timer->timerid);
}

int sio_timer_destory(struct sio_timer *timer)
{
    SIO_COND_CHECK_RETURN_VAL(!timer, -1);

    const struct sio_timer_ops *ops = timer->ops;

    int ret = ops->destory(timer->timerid);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    free(timer);

    return 0;
}