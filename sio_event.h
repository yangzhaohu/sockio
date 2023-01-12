#ifndef SIO_EVENT_H_
#define SIO_EVENT_H_

enum SIO_EVENTS_OPT
{
    SIO_EV_OPT_ADD,
    SIO_EV_OPT_MOD,
    SIO_EV_OPT_DEL
};

enum SIO_EVENTS
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
};

struct sio_event_buffer
{
    char *ptr;
    int len;
};

struct sio_event_owner
{
    unsigned long int fd;
    void *ptr;
};

struct sio_event
{
    unsigned int events;
    struct sio_event_owner owner;
    struct sio_event_buffer buf;
};

#endif