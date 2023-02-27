#ifndef SIO_MPLEX_H_
#define SIO_MPLEX_H_

#include "sio_event.h"

enum SIO_MPLEX_TYPE
{
    SIO_MPLEX_SELECT,
    SIO_MPLEX_EPOLL,    // only support linux
    SIO_MPLEX_IOCP      // only support windows
};

struct sio_mplex_attr
{
    enum SIO_MPLEX_TYPE type;
};

struct sio_mplex;

#ifdef __cplusplus
extern "C" {
#endif

struct sio_mplex *sio_mplex_create(struct sio_mplex_attr *attr);

int sio_mplex_ctl(struct sio_mplex *mp, enum sio_events_opt op, int fd, struct sio_event *event);

int sio_mplex_wait(struct sio_mplex *mp, struct sio_event *event, int count);

int sio_mplex_close(struct sio_mplex *mp);

int sio_mplex_destory(struct sio_mplex *mp);

#ifdef __cplusplus
}
#endif

#endif