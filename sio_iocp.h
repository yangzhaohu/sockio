#ifndef SIO_IOCP_H_
#define SIO_IOCP_H_

#include "sio_event.h"

struct sio_mplex_ctx;

#ifdef __cplusplus
extern "C" {
#endif

struct sio_mplex_ctx *sio_mplex_iocp_open(void);

int sio_mplex_iocp_ctl(struct sio_mplex_ctx *ctx, int op, int fd, struct sio_event *event);

int sio_mplex_iocp_wait(struct sio_mplex_ctx *ctx, struct sio_event *event, int count);

int sio_mplex_iocp_close(struct sio_mplex_ctx *ctx);

#ifdef __cplusplus
}
#endif

#endif