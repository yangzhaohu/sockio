#ifndef SIO_SELECT_H_
#define SIO_SELECT_H_

#include "sio_event.h"

#ifdef __cplusplus
extern "C" {
#endif

struct sio_mplex_ctx *sio_select_create(void);

int sio_select_ctl(struct sio_mplex_ctx *ctx, int op, sio_fd_t fd, struct sio_event *event);

int sio_select_wait(struct sio_mplex_ctx *ctx, struct sio_event *event, int count);

int sio_select_close(struct sio_mplex_ctx *ctx);

int sio_select_destory(struct sio_mplex_ctx *ctx);

#ifdef __cplusplus
}
#endif

#endif