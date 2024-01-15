#ifndef SIO_EVENT_H_
#define SIO_EVENT_H_

#include "sio_def.h"

enum sio_events_opt
{
    SIO_EV_OPT_ADD,
    SIO_EV_OPT_MOD,
    SIO_EV_OPT_DEL
};

enum sio_events
{
    SIO_EVENTS_NONE = 0,
    
    SIO_EVENTS_IN = 0x01,
    SIO_EVENTS_OUT = 0x02,
    
    SIO_EVENTS_ERR = 0x04,

    SIO_EVENTS_HUP = 0x10, // SHUT_RDWD
    SIO_EVENTS_RDHUP = 0x20,

    SIO_EVENTS_ONESHOT = 0x100, // EPOLL
    SIO_EVENTS_ET = 0x200, // EPOLL

    SIO_EVENTS_ASYNC_ACCEPT = 0x1000,
    SIO_EVENTS_ASYNC_READ = 0x2000,
    SIO_EVENTS_ASYNC_WRITE = 0x4000,
    SIO_EVENTS_ASYNC_ACCEPT_RES = 0x8000,

    SIO_EVENTS_HANDSHAKE_READ = 0x10000,
    SIO_EVENTS_HANDSHAKE_WRITE = 0x20000,
    SIO_EVENTS_ASYNC_HANDSHAKE_READ = 0x40000,
    SIO_EVENTS_ASYNC_HANDSHAKE_WRITE = 0x80000
};

typedef void * sio_aiobuf;

struct sio_evbuf
{
    sio_aiobuf ptr;
    int len;
};

struct sio_event
{
    unsigned int events;
    void *pri;
    int res;
    struct sio_evbuf buf;
};

int sio_aioctx_size();

sio_aiobuf sio_aiobuf_alloc(unsigned long size);

void *sio_aiobuf_aioctx_ptr(sio_aiobuf buf);

void sio_aiobuf_free(sio_aiobuf buf);

#endif