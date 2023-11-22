#ifndef SIO_AIO_H_
#define SIO_AIO_H_

#include "sio_def.h"

struct sio_event;

#ifdef __cplusplus
extern "C" {
#endif

int sio_aio_accept(sio_fd_t sfd, sio_fd_t fd, struct sio_event *event);

int sio_aio_recv(sio_fd_t fd, struct sio_event *event);

int sio_aio_send(sio_fd_t fd, struct sio_event *event);

#ifdef __cplusplus
}
#endif

#endif