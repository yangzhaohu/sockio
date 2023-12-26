#ifndef SIO_AIOSEQ_H_
#define SIO_AIOSEQ_H_

struct sio_aioseq
{
    void *ptr;
    char *buf;
    int len;
    unsigned int events;
    unsigned long flags;
};

#endif