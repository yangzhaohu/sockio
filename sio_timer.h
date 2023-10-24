#ifndef SIO_TIMER_H_
#define SIO_TIMER_H_

struct sio_timer;

#ifdef __cplusplus
extern "C" {
#endif

struct sio_timer *sio_timer_create(void *(*routine)(void *arg), void *arg);

int sio_timer_start(struct sio_timer *timer, unsigned int udelay, unsigned int uperiod);

int sio_timer_stop(struct sio_timer *timer);

int sio_timer_destory(struct sio_timer *timer);

#ifdef __cplusplus
}
#endif

#endif