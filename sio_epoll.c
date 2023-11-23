#include "sio_epoll.h"
#include <string.h>
#include <stdlib.h>
#ifdef LINUX
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <errno.h>
#endif
#include "sio_common.h"
#include "sio_mplex_pri.h"
#include "sio_event.h"
#include "sio_log.h"

struct sio_mplex_epoll_pri
{
    int notifyfd;
};

#define sio_mplex_get_efd(ctx) ctx->fd.nfd

#define sio_mplex_op_trans_to_epoll_op(op)                      \
    if (op == SIO_EV_OPT_ADD) { op = EPOLL_CTL_ADD; }           \
    else if (op == SIO_EV_OPT_MOD) { op = EPOLL_CTL_MOD; }      \
    else { op = EPOLL_CTL_DEL; }    

#define sio_event_trans_to_epoll_event(ori, dst)                \
    if (ori & SIO_EVENTS_IN) { dst |= EPOLLIN; }                \
    if (ori & SIO_EVENTS_OUT) { dst |= EPOLLOUT; }              \
    if (ori & SIO_EVENTS_ET) { dst |= EPOLLET; }                \
    if (ori & SIO_EVENTS_ONESHOT) { dst |= EPOLLONESHOT; }

#define epoll_event_trans_to_sio_event(ori, dst)            \
    if (ori & EPOLLIN) { dst |= SIO_EVENTS_IN; }            \
    if (ori & EPOLLOUT) { dst |= SIO_EVENTS_OUT; }          \
    if (ori & EPOLLERR) { dst |= SIO_EVENTS_ERR; }          \
    if (ori & EPOLLHUP) { dst |= SIO_EVENTS_HUP; }          \
    if (ori & EPOLLRDHUP) { dst |= SIO_EVENTS_RDHUP; }

#ifdef LINUX

static inline
int sio_eventfd_create()
{
    int fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    return fd;
}

static inline
int sio_eventfd_close(struct sio_mplex_epoll_pri *epp)
{
    int fd = epp->notifyfd;
    int ret = close(fd);

    return ret;
}

static inline
int sio_mplex_epoll_ctl_imp(int efd, int op, int fd, struct sio_event *event)
{
    struct epoll_event ep_ev = { 0 };
    sio_event_trans_to_epoll_event(event->events, ep_ev.events);
    ep_ev.data.ptr = event->owner.pri;
    sio_mplex_op_trans_to_epoll_op(op);
    return epoll_ctl(efd, op, fd, &ep_ev);
}

struct sio_mplex_ctx *sio_mplex_epoll_create(void)
{
    struct sio_mplex_ctx *ctx = malloc(sizeof(struct sio_mplex_ctx));
    SIO_COND_CHECK_RETURN_VAL(!ctx, NULL);

    memset(ctx, 0, sizeof(struct sio_mplex_ctx));

    int fd = epoll_create(256);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(fd == -1, NULL,
        free(ctx));

    struct sio_mplex_epoll_pri *epp = malloc(sizeof(struct sio_mplex_epoll_pri));
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(epp == NULL, NULL,
        free(ctx));

    memset(epp, 0, sizeof(struct sio_mplex_epoll_pri));

    int notifyfd = sio_eventfd_create();
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(notifyfd == -1, NULL,
        free(ctx),
        free(epp));

    struct sio_event ev = { 0 };
    ev.events = SIO_EVENTS_IN;
    ev.owner.pri = ctx;
    int ret = sio_mplex_epoll_ctl_imp(fd, SIO_EV_OPT_ADD, notifyfd, &ev);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, NULL,
        close(notifyfd),
        free(ctx),
        free(epp));

    epp->notifyfd = notifyfd;

    ctx->fd.nfd = fd;
    ctx->pri = epp;
    return ctx;
}

int sio_mplex_epoll_ctl(struct sio_mplex_ctx *ctx, int op, sio_fd_t fd, struct sio_event *event)
{
    int efd = sio_mplex_get_efd(ctx);
    return sio_mplex_epoll_ctl_imp(efd, op, fd, event);
}

static inline
int sio_eventfd_read(struct sio_mplex_epoll_pri *epp)
{
    int notifyfd = epp->notifyfd;
    unsigned long long val = 0;
    int ret = read(notifyfd, &val, sizeof(unsigned long long));
    return ret;
}

static inline
int sio_eventfd_notify(struct sio_mplex_epoll_pri *epp)
{
    int notifyfd = epp->notifyfd;
    unsigned long long val = 1;
    int ret = write(notifyfd, &val, sizeof(unsigned long long));
    return ret;
}

int sio_mplex_epoll_wait(struct sio_mplex_ctx *ctx, struct sio_event *event, int count)
{
    struct epoll_event ep_ev[count];
    struct sio_mplex_epoll_pri *epp = ctx->pri;

    int fd = sio_mplex_get_efd(ctx);
    int ret = epoll_wait(fd, ep_ev, count, -1);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        SIO_LOGE("epoll_wait errno: %d\n", errno));

    for (int i = 0; i < ret; i++) {
        SIO_COND_CHECK_CALLOPS_BREAK(ep_ev[i].data.ptr == ctx,
            sio_eventfd_read(epp),
            sio_eventfd_notify(epp),
            ret = -1);

        unsigned int epevents = ep_ev[i].events;
        unsigned int events = 0;
        epoll_event_trans_to_sio_event(epevents, events);
        event[i].events = events;
        event[i].owner.pri = ep_ev[i].data.ptr;
    }

    return ret;
}

int sio_mplex_epoll_close(struct sio_mplex_ctx *ctx)
{
    struct sio_mplex_epoll_pri *epp = ctx->pri;
    int ret = sio_eventfd_notify(epp);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        SIO_LOGE("epoll eventfd write failed\n"));

    return 0;
}

int sio_mplex_epoll_destory(struct sio_mplex_ctx *ctx)
{
    struct sio_mplex_epoll_pri *epp = ctx->pri;
    sio_eventfd_close(epp);

    int fd = sio_mplex_get_efd(ctx);
    int ret = close(fd);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    free(ctx);
    return 0;
}

#else

struct sio_mplex_ctx *sio_mplex_epoll_create(void)
{
    return NULL;
}

int sio_mplex_epoll_ctl(struct sio_mplex_ctx *ctx, int op, sio_fd_t fd, struct sio_event *event)
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

int sio_mplex_epoll_destory(struct sio_mplex_ctx *ctx)
{
    return -1;
}

#endif