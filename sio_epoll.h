#ifndef SIO_EPOLL_H_
#define SIO_EPOLL_H_

#include "sio_event.h"

struct sio_mplex_ctx;

#ifdef __cplusplus
extern "C" {
#endif

struct sio_mplex_ctx *sio_mplex_epoll_create(void);

int sio_mplex_epoll_ctl(struct sio_mplex_ctx *ctx, int op, sio_fd_t fd, struct sio_event *event);

int sio_mplex_epoll_wait(struct sio_mplex_ctx *ctx, struct sio_event *event, int count);

int sio_mplex_epoll_close(struct sio_mplex_ctx *ctx);

int sio_mplex_epoll_destory(struct sio_mplex_ctx *ctx);

#ifdef __cplusplus
}
#endif

#endif