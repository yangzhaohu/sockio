#ifndef SIO_MPLEX_PRI_H_
#define SIO_MPLEX_PRI_H_

#include "sio_def.h"

union sio_mplex_fd
{
    int nfd;
    void *pfd;
};

struct sio_mplex_ctx
{
    union sio_mplex_fd fd;
    void *pri;
};

struct sio_mplex_ops
{
    struct sio_mplex_ctx * (*create)(void);
    int (*ctl)(struct sio_mplex_ctx *mpctx, int op, sio_fd_t fd, struct sio_event *event);
    int (*wait)(struct sio_mplex_ctx *mpctx, struct sio_event *event, int count);
    int (*close)(struct sio_mplex_ctx *mpctx);
    int (*destory)(struct sio_mplex_ctx *mpctx);
};

#endif