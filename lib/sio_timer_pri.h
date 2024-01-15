#ifndef SIO_TIMER_PRI_H_
#define SIO_TIMER_PRI_H_

struct sio_timer_pri
{
    void *(*routine)(void *arg);
    void *arg;
};

typedef void * sio_timer_t;

enum sio_timer_mode
{
    SIO_TIMER_PERIOD,
    SIO_TIMER_SINGLESHOT
};

struct sio_timer_ops
{
    sio_timer_t (*create)(struct sio_timer_pri *pri);
    int (*start)(sio_timer_t timer, unsigned long int udelay, unsigned long int uperiod);
    int (*stop)(sio_timer_t timer);
    int (*destory)(sio_timer_t timer);
};

#endif