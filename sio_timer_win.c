#include "sio_timer_win.h"

#ifdef WIN32

sio_timer_t sio_timer_win_create(struct sio_timer_pri *pri)
{
    return 0;
}

int sio_timer_win_start(sio_timer_t timer, unsigned long int udelay, unsigned long int uperiod)
{
    return -1;
}

int sio_timer_win_stop(sio_timer_t timer)
{
    return -1;
}

int sio_timer_win_destory(sio_timer_t timer)
{
    return -1;
}

#endif