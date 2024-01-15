#ifndef SIO_TIMER_WIN_H_
#define SIO_TIMER_WIN_H_

#include "sio_timer_pri.h"

#ifdef __cplusplus
extern "C" {
#endif

sio_timer_t sio_timer_win_create(struct sio_timer_pri *pri);

int sio_timer_win_start(sio_timer_t timer, unsigned long int udelay, unsigned long int uperiod);

int sio_timer_win_stop(sio_timer_t timer);

int sio_timer_win_destory(sio_timer_t timer);

#ifdef __cplusplus
}
#endif

#endif