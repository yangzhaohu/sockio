#include "sio_timer_posix.h"
#include <stdlib.h>
#include <string.h>
#ifdef LINUX
#include <signal.h>
#include <time.h>
#include <unistd.h>
#endif
#include "sio_common.h"
#include "sio_global.h"
#include "sio_thread.h"
#include "sio_log.h"

#ifdef LINUX

struct sio_timer_posix
{
    timer_t timerid;
};

#define SIO_TIMER_POSIX_CAST(timer) ((struct sio_timer_posix *)timer)

static inline
void sio_timer_posix_routine(sigval_t sigval)
{
    struct sio_timer_pri *pri = sigval.sival_ptr;
    if (pri && pri->routine) {
        pri->routine(pri->arg);
    }
}

static inline
timer_t sio_timer_posix_timer_create(struct sio_timer_pri *pri)
{
    timer_t timer = NULL;
    struct sigevent sev = { 0 };
    sev.sigev_notify = SIGEV_THREAD;
    // sev.sigev_signo = SIGALRM;
    sev.sigev_notify_function = sio_timer_posix_routine;
    sev.sigev_value.sival_ptr = pri;

    int ret = timer_create(CLOCK_MONOTONIC, &sev, &timer);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, NULL);

    return timer;
}

sio_timer_t sio_timer_posix_create(struct sio_timer_pri *pri)
{
    struct sio_timer_posix *timer = malloc(sizeof(struct sio_timer_posix));
    SIO_COND_CHECK_RETURN_VAL(!timer, NULL);

    memset(timer, 0, sizeof(struct sio_timer_posix));

    timer_t timerid = sio_timer_posix_timer_create(pri);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(timerid == NULL, NULL,
        free(timer));

    timer->timerid = timerid;

    return timer;
}

static inline
int sio_timer_posix_settime(timer_t timerid, unsigned long int udelay, unsigned long int uperiod)
{
    struct itimerspec tspec = { 0 };
    tspec.it_value.tv_sec = udelay / 1000000;
    tspec.it_value.tv_nsec = (udelay % 1000000) * 1000;
    tspec.it_interval.tv_sec = uperiod / 1000000;
    tspec.it_interval.tv_nsec = (uperiod % 1000000) * 1000;

    return timer_settime(timerid, 0, &tspec, NULL);
}

int sio_timer_posix_start(sio_timer_t timer, unsigned long int udelay, unsigned long int uperiod)
{
    return sio_timer_posix_settime(SIO_TIMER_POSIX_CAST(timer)->timerid, udelay, uperiod);
}

int sio_timer_posix_stop(sio_timer_t timer)
{
    return sio_timer_posix_settime(SIO_TIMER_POSIX_CAST(timer)->timerid, 0, 0);
}

int sio_timer_posix_destory(sio_timer_t timer)
{
    if (SIO_TIMER_POSIX_CAST(timer)->timerid) {
        timer_delete(SIO_TIMER_POSIX_CAST(timer)->timerid);
    }

    free(timer);

    return 0;
}

#endif