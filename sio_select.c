#include "sio_select.h"
#include <stdlib.h>
#include <string.h>
#include "sio_common.h"
#include "sio_mplex_pri.h"
#include "sio_select_pri.h"
#include "sio_socket_pri.h"
#include "sio_event.h"
#include "sio_mutex.h"
#include "sio_atomic.h"
#include "sio_log.h"

#define SIO_SELECT_SOCK_NOTIFY_PORT 65535

#define SIO_SELECT_NOTIFY_MODE char
#define SIO_SELECT_NOTIFY_EXIT 0x01
#define SIO_SELECT_NOTIFY_POLL 0x02

struct sio_evfds
{
    sio_socket_t maxfd;
    struct sio_event evs[SIO_FD_SETSIZE];
    struct sio_rwfds fds;
};

struct sio_fdout
{
    int pends;
    struct sio_rwfds fds;
};

struct sio_select
{
    struct sio_mplex_ctx ctx;
    sio_socket_t notify_pair[2];
    unsigned short notify_port;

    sio_mutex mutex;
    struct sio_evfds evfs;
};

static int g_notify_port = SIO_SELECT_SOCK_NOTIFY_PORT;

#ifdef WIN32
__declspec(thread) struct sio_fdout tls_fout= { 0 };
#else
__thread struct sio_fdout tls_fout= { 0 };
#endif

static inline
int sio_mplex_select_notify_read(struct sio_select *slt);

static inline 
void sio_select_set_rwfds(struct sio_rwfds *rwfds, sio_fd_t fd, enum sio_events events)
{
    if (events & SIO_EVENTS_IN) {
        SIO_FD_SET(fd, &rwfds->rfds);
    } else {
        SIO_FD_CLR(fd, &rwfds->rfds);
    }

    if (events & SIO_EVENTS_OUT) {
        SIO_FD_SET(fd, &rwfds->wfds); 
    } else {
        SIO_FD_CLR(fd, &rwfds->wfds); 
    }
}

static inline
void sio_select_set_evs(struct sio_event *evs, sio_fd_t fd, struct sio_event *event)
{
#ifndef WIN32
    evs[fd] = *event;

#else
    int del = 0;
    if ((event->events & (SIO_EVENTS_IN | SIO_EVENTS_OUT)) == 0) {
        del = 1;
    }

    int i = 0;
    for (; i < SIO_FD_SETSIZE; i++) {
        if (evs[i].owner.fd == fd || evs[i].owner.fd == 0) {
            break;
        }
    }

    if (del == 1) {
        memset(&evs[i], 0, sizeof(struct sio_event));
        for (int j = i; j < SIO_FD_SETSIZE - 1 && evs[j + 1].owner.fd != 0; j++) {
            evs[j] = evs[j + 1];
        }
        return;
    }

    event->owner.fd = fd; // for windows platform
    evs[i] = *event;
#endif
}

static inline
void sio_select_set_evfds(struct sio_evfds *efs, sio_fd_t fd, struct sio_event *event)
{
    sio_select_set_evs(efs->evs, fd, event);
    sio_select_set_rwfds(&efs->fds, fd, event->events);
}

static inline
void sio_select_in_clr(struct sio_evfds *efs, sio_fd_t fd)
{
    struct sio_event event = { 0 };
    sio_select_set_evs(efs->evs, fd, &event);
    sio_select_set_rwfds(&efs->fds, fd, SIO_EVENTS_NONE);
}

static inline
void sio_select_check_modify_maxfd(struct sio_evfds *efs, sio_fd_t fd)
{
    int i = efs->maxfd < fd ? fd : efs->maxfd;
    struct sio_rwfds *rwfds = &efs->fds;
    for (; i >= 0; i--) {
        if (FD_ISSET(i, &rwfds->rfds) ||
            FD_ISSET(i, &rwfds->wfds)) {
            break;
        }
    }

    efs->maxfd = i;
}

static inline
void sio_select_set_event(struct sio_select *slt, sio_fd_t fd, struct sio_event *event)
{
    sio_mutex_lock(&slt->mutex);
    struct sio_evfds *efs = &slt->evfs;
    sio_select_set_evfds(efs, fd, event);
#ifndef WIN32
    sio_select_check_modify_maxfd(efs, fd);
#endif
    sio_mutex_unlock(&slt->mutex);
}

static inline
void sio_select_clr_event(struct sio_select *slt, sio_fd_t fd)
{
    sio_mutex_lock(&slt->mutex);
    struct sio_evfds *efs = &slt->evfs;
    sio_select_in_clr(efs, fd);
#ifndef WIN32
    sio_select_check_modify_maxfd(efs, fd);
#endif
    sio_mutex_unlock(&slt->mutex);
}


#define sio_in_event_owner(fd) slt->evfs.evs[fd].owner

#ifdef WIN32

static inline
int sio_in_event_owner_index(struct sio_select *slt, sio_fd_t fd)
{
    struct sio_evfds *efs = &slt->evfs;
    for (int i = 0; i < SIO_FD_SETSIZE; i++) {
        struct sio_event_owner *owner = &(efs->evs[i].owner);
        if (owner->fd == fd) {
            return i;
        }
    }

    return -1;
}

#define sio_out_event_get(fdset, evtype) \
    while (fdset->fd_count > 0 && resolved < count) { \
        sio_fd_t fd = fdset->fd_array[fdset->fd_count - 1]; \
        int index = sio_in_event_owner_index(slt, fd); \
        if (index == -1) { \
            fdset->fd_count--; \
            continue; \
        } else if (fd == slt->notify_pair[0]) { \
            int mode = sio_mplex_select_notify_read(slt); \
            if (mode == SIO_SELECT_NOTIFY_EXIT) { \
                resolved = -1; \
                break; \
            } else if (mode == SIO_SELECT_NOTIFY_POLL) { \
                fdset->fd_count--; \
                continue; \
            } \
        } \
        event[resolved].events |= evtype; \
        event[resolved].owner = sio_in_event_owner(index); \
        resolved++; \
        fdset->fd_count--; \
    }

#endif

static inline
int sio_select_event(struct sio_select *slt, struct sio_event *event, int count)
{
    sio_mutex_lock(&slt->mutex);
    struct sio_fdout *out = &tls_fout;
    sio_fd_set *rfds = &out->fds.rfds;
    sio_fd_set *wfds = &out->fds.wfds;
    int resolved = 0;

#ifndef WIN32
    int pends = out->pends;
    while (pends > 0 && resolved < count) {
        unsigned int events = 0;
        if (SIO_FD_ISSET(pends, rfds)) {
            if (pends == slt->notify_pair[0]) {
                int mode = sio_mplex_select_notify_read(slt);
                if (mode == SIO_SELECT_NOTIFY_EXIT) {
                    resolved = -1;
                    break;
                } else if (mode == SIO_SELECT_NOTIFY_POLL) {
                    pends--;
                    continue;
                }
            }
            events |= SIO_EVENTS_IN;
        }
        if (SIO_FD_ISSET(pends, wfds)) {
            events |= SIO_EVENTS_OUT;
        }
        if (events != 0) {
            event[resolved].events = events;
            event[resolved].owner = sio_in_event_owner(pends);
            resolved++;
        }

        pends--;
    }

    out->pends = pends;
#else
    sio_out_event_get(wfds, SIO_EVENTS_OUT);
    sio_out_event_get(rfds, SIO_EVENTS_IN);
#endif
    sio_mutex_unlock(&slt->mutex);

    return resolved;
}

static inline
int sio_select_check_out_bounds(struct sio_select *slt, sio_fd_t fd)
{
#ifndef WIN32
    SIO_COND_CHECK_RETURN_VAL(fd >= SIO_FD_SETSIZE, -1);
#else
    sio_mutex_lock(&slt->mutex);
    struct sio_evfds *efs = &slt->evfs;
    sio_fd_set *rfds = &efs->fds.rfds;
    sio_fd_set *wfds = &efs->fds.wfds;

    int index = sio_in_event_owner_index(slt, fd);
    if (index == -1 && (rfds->fd_count > SIO_FD_SETSIZE ||
        wfds->fd_count > SIO_FD_SETSIZE)) {
        return -1;
    }
    sio_mutex_unlock(&slt->mutex);
#endif

    return 0;
}

static inline
int sio_mplex_select_notifypair_create(struct sio_select *slt)
{
    sio_socket_t pair1 = socket(PF_INET, SOCK_DGRAM, 0);
    SIO_COND_CHECK_RETURN_VAL(pair1 == -1, -1);

    unsigned short port = slt->notify_port;

    struct sockaddr_in addr_in = { 0 };
    addr_in.sin_family = PF_INET;
    addr_in.sin_port = htons(port);
    addr_in.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int ret = bind(pair1, (struct sockaddr *)&addr_in, sizeof(addr_in));
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        CLOSE(pair1));

    sio_socket_t pair2 = socket(PF_INET, SOCK_DGRAM, 0);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(pair2 == -1, -1,
        CLOSE(pair1));

    addr_in.sin_port = htons(port - 1);
    ret = bind(pair2, (struct sockaddr *)&addr_in, sizeof(addr_in));
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        CLOSE(pair1),
        CLOSE(pair2));

    slt->notify_pair[0] = pair1;
    slt->notify_pair[1] = pair2;

    return 0;
}

static inline
int sio_mplex_select_notify(struct sio_select *slt, SIO_SELECT_NOTIFY_MODE mode)
{
    struct sockaddr_in addr = { 0 };
    addr.sin_family = PF_INET;
    addr.sin_port = htons(slt->notify_port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    int ret = sendto(slt->notify_pair[1], &mode, sizeof(char), 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

    return ret;
}

static inline
int sio_mplex_select_notify_read(struct sio_select *slt)
{
    SIO_SELECT_NOTIFY_MODE mode = 0;
    struct sockaddr_in addr = { 0 };
    unsigned int l = sizeof(addr);
    int ret = recvfrom(slt->notify_pair[0], &mode, sizeof(char), 0, (struct sockaddr *)&addr, &l);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, SIO_SELECT_NOTIFY_POLL,
        SIO_LOGE("notify_read failed\n"));

    // SIO_LOGE("notify read: %d\n", mode);
    SIO_COND_CHECK_RETURN_VAL(addr.sin_addr.s_addr != htonl(INADDR_LOOPBACK) || 
        addr.sin_port != htons(slt->notify_port - 1), SIO_SELECT_NOTIFY_POLL);

    return mode;
}

struct sio_mplex_ctx *sio_mplex_select_create(void)
{
    struct sio_select *slt = malloc(sizeof(struct sio_select));
    SIO_COND_CHECK_RETURN_VAL(!slt, NULL);

    memset(slt, 0, sizeof(struct sio_select));

    SIO_COND_CHECK_CALLOPS_RETURN_VAL(sio_int_fetch_and_sub(&g_notify_port, 0) <= 4, NULL,
        free(slt));

    slt->notify_port = sio_int_fetch_and_sub(&g_notify_port, 2);

    int ret = sio_mplex_select_notifypair_create(slt);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, NULL,
        sio_int_fetch_and_add(&g_notify_port, 2),
        free(slt));

    sio_mutex_init(&slt->mutex);

    struct sio_event event = { .events = SIO_EVENTS_IN };
    sio_select_set_event(slt, slt->notify_pair[0], &event);

    return &slt->ctx;
}

int sio_mplex_select_ctl(struct sio_mplex_ctx *ctx, int op, sio_fd_t fd, struct sio_event *event)
{
    struct sio_select *slt = (struct sio_select *)ctx;
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(sio_select_check_out_bounds(slt, fd) == -1, -1,
        SIO_LOGE("socket descriptor %llu out of bounds\n", (sio_uint64_t)fd));

    if (op == SIO_EV_OPT_ADD || op == SIO_EV_OPT_MOD) {
        sio_select_set_event(slt, fd, event);
    } else if (op == SIO_EV_OPT_DEL) {
        sio_select_clr_event(slt, fd);
    }

    sio_mplex_select_notify(slt, SIO_SELECT_NOTIFY_POLL); // notify唤醒select

    return 0;
}

int sio_mplex_select_wait(struct sio_mplex_ctx *ctx, struct sio_event *event, int count)
{
    struct sio_select *slt = (struct sio_select *)ctx;
    int resolved = sio_select_event(slt, event, count);
    SIO_COND_CHECK_RETURN_VAL(resolved > 0, resolved);

    sio_mutex_lock(&slt->mutex);
    struct sio_evfds *efs = &slt->evfs;
    struct sio_fdout *out = &tls_fout;
    memcpy(&out->fds, &efs->fds, sizeof(struct sio_rwfds));

    sio_socket_t maxfd = efs->maxfd; // for linux platform
    out->pends = maxfd;  // for linux platform

    sio_fd_set *rfds = &out->fds.rfds;
    sio_fd_set *wfds = &out->fds.wfds;
    sio_mutex_unlock(&slt->mutex);

    int ret = select(maxfd + 1, (fd_set *)rfds, (fd_set *)wfds, NULL, NULL/*&timeout*/);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    resolved = sio_select_event(slt, event, count);
    return resolved;
}

int sio_mplex_select_close(struct sio_mplex_ctx *ctx)
{
    struct sio_select *slt = (struct sio_select *)ctx;

    int ret = sio_mplex_select_notify(slt, SIO_SELECT_NOTIFY_EXIT);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        SIO_LOGE("select notify write failed\n"));

    return ret;
}

int sio_mplex_select_destory(struct sio_mplex_ctx *ctx)
{
    free(ctx);
    return 0;
}