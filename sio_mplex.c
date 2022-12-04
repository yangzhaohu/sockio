#include "sio_mplex.h"
#include <stdlib.h>
#include <string.h>
#include "sio_common.h"
#include "sio_mplex_pri.h"
#include "sio_select.h"
#include "sio_epoll.h"
#include "sio_iocp.h"
#include "sio_log.h"

struct sio_mplex
{
    struct sio_mplex_attr attr;
    struct sio_mplex_ctx *ctx;
    const struct sio_mplex_ops *ops;
};

static const 
struct sio_mplex_ops g_mplex_select_ops =
{
    .open = sio_mplex_select_open,
    .ctl  = sio_mplex_select_ctl,
    .wait = sio_mplex_select_wait,
    .close = sio_mplex_select_close
};

static const 
struct sio_mplex_ops g_mplex_epoll_ops =
{
    .open = sio_mplex_epoll_open,
    .ctl  = sio_mplex_epoll_ctl,
    .wait = sio_mplex_epoll_wait,
    .close = sio_mplex_epoll_close
};

static const 
struct sio_mplex_ops g_mplex_iocp_ops =
{
    .open = sio_mplex_iocp_open,
    .ctl  = sio_mplex_iocp_ctl,
    .wait = sio_mplex_iocp_wait,
    .close = sio_mplex_iocp_close
};

static const
struct sio_mplex_ops *g_mplex[] = 
{
    [SIO_MPLEX_SELECT] = &g_mplex_select_ops,
    [SIO_MPLEX_EPOLL]  = &g_mplex_epoll_ops,
    [SIO_MPLEX_IOCP]   = &g_mplex_iocp_ops,
};

#define SIO_MPLEX_PLEXER_SIZE (sizeof(g_mplex) / sizeof(struct sio_mplex_ops *))

static inline 
void sio_mplex_release(struct sio_mplex *mp)
{
    free(mp);
}

struct sio_mplex *sio_mplex_create(struct sio_mplex_attr *attr)
{   
    SIO_COND_CHECK_RETURN_VAL(!attr, NULL);

    enum SIO_MPLEX_TYPE type = attr->type;
    SIO_COND_CHECK_RETURN_VAL(type < 0 || type >= SIO_MPLEX_PLEXER_SIZE, NULL);

    const struct sio_mplex_ops *ops = g_mplex[type];
    SIO_COND_CHECK_RETURN_VAL(ops == NULL, NULL);

    struct sio_mplex *mp = malloc(sizeof(struct sio_mplex));
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!mp, NULL, 
        SIO_LOGE("sio_mplex malloc failed\n"));

    memset(mp, 0, sizeof(struct sio_mplex));

    memcpy(&mp->attr, attr, sizeof(struct sio_mplex_attr));
    
    mp->ops = ops;

    struct sio_mplex_ctx *ctx = ops->open();
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!ctx, NULL,
        SIO_LOGE("sio_mplex open failed\n"),
        free(mp));

    mp->ctx = ctx;

    return mp;
}

int sio_mplex_ctl(struct sio_mplex *mp, enum SIO_EVENTS_OPT op, int fd, struct sio_event *event)
{
    SIO_COND_CHECK_RETURN_VAL(!mp || !event, -1);

    const struct sio_mplex_ops *ops = mp->ops;
    return ops->ctl(mp->ctx, op, fd, event);
}

int sio_mplex_wait(struct sio_mplex *mp, struct sio_event *event, int count)
{
    SIO_COND_CHECK_RETURN_VAL(!mp || !event || count <= 0, -1);

    const struct sio_mplex_ops *ops = mp->ops;
    return ops->wait(mp->ctx, event, count);
}

int sio_mplex_close(struct sio_mplex *mp)
{
    SIO_COND_CHECK_RETURN_VAL(!mp, -1);

    const struct sio_mplex_ops *ops = mp->ops;

    int ret = ops->close(mp->ctx);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    sio_mplex_release(mp);
    return 0;
}