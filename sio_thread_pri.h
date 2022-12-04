#ifndef SIO_THREAD_PRI_H_
#define SIO_THREAD_PRI_H_

struct sio_thread_pri
{
    void *(*routine)(void *arg);
    void *arg;
};

struct sio_thread_ops
{
    unsigned long int (*create)(struct sio_thread_pri *pri);
    int (*self)(void);
    // int (*st_join)(void);
    int (*close)(unsigned long int tid);
};

#endif