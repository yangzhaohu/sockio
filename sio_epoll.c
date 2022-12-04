#include "sio_epoll.h"
#include <string.h>
#include <stdlib.h>
#ifdef LINUX
#include <unistd.h>
#include <sys/epoll.h>
#endif
#include "sio_common.h"
#include "sio_mplex_pri.h"
#include "sio_event.h"
#include "sio_log.h"

#define sio_mplex_get_efd(ctx) ctx->fd.nfd

#define sio_event_trans_to_epoll_event(ori, dst)                \
    if (ori & SIO_EVENTS_IN) { dst |= EPOLLIN; }                \
    if (ori & SIO_EVENTS_OUT) { dst |= EPOLLOUT; }              \
    if (ori & SIO_EVENTS_ET) { dst |= EPOLLET; }                \
    if (ori & SIO_EVENTS_ONESHOT) { dst |= EPOLLONESHOT; }

#define epoll_event_trans_to_sio_event(ori, dst)            \
    if (ori & EPOLLIN) { dst |= SIO_EVENTS_IN; }            \
    if (ori & EPOLLOUT) { dst |= SIO_EVENTS_OUT; }          \
    if (ori & EPOLLERR) { dst |= SIO_EVENTS_ERR; }

#ifdef LINUX

struct sio_mplex_ctx *sio_mplex_epoll_open(void)
{
    struct sio_mplex_ctx *ctx = malloc(sizeof(struct sio_mplex_ctx));
    SIO_COND_CHECK_RETURN_VAL(!ctx, NULL);

    memset(ctx, 0, sizeof(struct sio_mplex_ctx));

    int fd = epoll_create(256);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(fd == -1, NULL,
        free(ctx));

    ctx->fd.nfd = fd;
    return ctx;
}

int sio_mplex_epoll_ctl(struct sio_mplex_ctx *ctx, int op, int fd, struct sio_event *event)
{
    struct epoll_event ep_ev = { 0 };
    sio_event_trans_to_epoll_event(event->events, ep_ev.events);
    ep_ev.data.ptr = event->owner.ptr;
    int efd = sio_mplex_get_efd(ctx);
    return epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ep_ev);
}

int sio_mplex_epoll_wait(struct sio_mplex_ctx *ctx, struct sio_event *event, int count)
{
    struct epoll_event ep_ev[count];
    int fd = sio_mplex_get_efd(ctx);
    int cnt = epoll_wait(fd, ep_ev, count, -1);

    for (int i = 0; i < cnt; i++) {
        unsigned int epevents = ep_ev[i].events;
        unsigned int events = 0;
        epoll_event_trans_to_sio_event(epevents, events);
        event[i].events = events;
        event[i].owner.ptr = ep_ev[i].data.ptr;
    }
    return cnt;
}

int sio_mplex_epoll_close(struct sio_mplex_ctx *ctx)
{
    int fd = sio_mplex_get_efd(ctx);

    int ret = close(fd);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    free(ctx);
    return 0;
}

#else

struct sio_mplex_ctx *sio_mplex_epoll_open(void)
{
    return NULL;
}

int sio_mplex_epoll_ctl(struct sio_mplex_ctx *ctx, int op, int fd, struct sio_event *event)
{
    return -1;
}

int sio_mplex_epoll_wait(struct sio_mplex_ctx *ctx, struct sio_event *event, int count)
{
    return -1;
}

int sio_mplex_epoll_close(struct sio_mplex_ctx *ctx)
{
    return -1;
}

#endif