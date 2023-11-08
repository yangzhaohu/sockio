#include "sio_select.h"
#include <stdlib.h>
#include <string.h>
#include "sio_common.h"
#include "sio_mplex_pri.h"
#include "sio_select_pri.h"
#include "sio_event.h"
#include "sio_mutex.h"
#include "sio_log.h"

#define SIO_SELECT_SOCK_NOTIFY_PORT 65535

struct sio_select_fin
{
    int maxfd;
    struct sio_event evs[FD_SETSIZE];
    struct sio_select_rwfds fds;
};

struct sio_select_fout
{
    int pends;
    struct sio_select_rwfds fds;
};

struct sio_select
{
    struct sio_mplex_ctx ctx;
    int notify_pair[2];

    sio_mutex mutex;
    struct sio_select_fin fin;
};

#ifdef WIN32
__declspec(thread) struct sio_select_fout tls_fout= { 0 };
#else
__thread struct sio_select_fout tls_fout= { 0 };
#endif

static inline
int sio_mplex_select_notify_read(struct sio_select *slt);

static inline 
void sio_select_in_fds_set(struct sio_select_rwfds *rwfds, sio_fd_t fd, enum sio_events events)
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
void sio_select_in_event_set(struct sio_event *evs, sio_fd_t fd, struct sio_event *event)
{
#ifndef WIN32
    evs[fd] = *event;

#else
    int del = 0;
    if ((event->events & SIO_EVENTS_IN) == 0 &&
        (event->events & SIO_EVENTS_OUT) == 0) {
        del = 1;
    }

    int i = 0;
    for (; i < SIO_FD_SETSIZE; i++) {
        if (evs[i].owner.fd == fd ||
            evs[i].owner.fd == 0) {
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
void sio_select_in_set(struct sio_select_fin *in, sio_fd_t fd, struct sio_event *event)
{
    sio_select_in_event_set(in->evs, fd, event);
    sio_select_in_fds_set(&in->fds, fd, event->events);
}

static inline
void sio_select_in_clr(struct sio_select_fin *in, sio_fd_t fd)
{
    struct sio_event event = { 0 };
    sio_select_in_event_set(in->evs, fd, &event);
    sio_select_in_fds_set(&in->fds, fd, SIO_EVENTS_NONE);
}

static inline
void sio_select_check_modify_maxfd(struct sio_select_fin *in, sio_fd_t fd)
{
    int i = in->maxfd < fd ? fd : in->maxfd;
    struct sio_select_rwfds *rwfds = &in->fds;
    for (; i >= 0; i--) {
        if (FD_ISSET(i, &rwfds->rfds) ||
            FD_ISSET(i, &rwfds->wfds)) {
            break;
        }
    }

    in->maxfd = i;
}

static inline
void sio_select_event_set(struct sio_select *slt, sio_fd_t fd, struct sio_event *event)
{
    sio_mutex_lock(&slt->mutex);
    struct sio_select_fin *in = &slt->fin;
    sio_select_in_set(in, fd, event);
#ifndef WIN32
    sio_select_check_modify_maxfd(in, fd);
#endif
    sio_mutex_unlock(&slt->mutex);
}

static inline
void sio_select_event_clr(struct sio_select *slt, sio_fd_t fd)
{
    sio_mutex_lock(&slt->mutex);
    struct sio_select_fin *in = &slt->fin;
    sio_select_in_clr(in, fd);
#ifndef WIN32
    sio_select_check_modify_maxfd(in, fd);
#endif
    sio_mutex_unlock(&slt->mutex);
}


#define sio_in_event_owner(fd) slt->fin.evs[fd].owner

#ifdef WIN32

static inline
int sio_in_event_owner_index(struct sio_select *slt, sio_fd_t fd)
{
    struct sio_select_fin *in = &slt->fin;
    for (int i = 0; i < SIO_FD_SETSIZE; i++) {
        struct sio_event_owner *owner = &(in->evs[i].owner);
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
            sio_mplex_select_notify_read(slt); \
            resolved = -1; \
            break; \
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
    struct sio_select_fout *out = &tls_fout;
    sio_fd_set *rfds = &out->fds.rfds;
    sio_fd_set *wfds = &out->fds.wfds;
    int resolved = 0;

#ifndef WIN32
    int pends = out->pends;
    while (pends > 0 && resolved < count) {
        unsigned int events = 0;
        if (SIO_FD_ISSET(pends, rfds)) {
            events |= SIO_EVENTS_IN;
            if (pends == slt->notify_pair[0]) {
                sio_mplex_select_notify_read(slt);
                resolved = -1;
                break;
            }
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
    struct sio_select_fin *in = &slt->fin;
    sio_fd_set *rfds = &in->fds.rfds;
    sio_fd_set *wfds = &in->fds.wfds;

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
    int pair1 = socket(PF_INET, SOCK_DGRAM, 0);
    SIO_COND_CHECK_RETURN_VAL(pair1 == -1, -1);

    struct sockaddr_in addr_in = { 0 };
    addr_in.sin_family = PF_INET;
    addr_in.sin_port = htons(SIO_SELECT_SOCK_NOTIFY_PORT);
    addr_in.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int ret = bind(pair1, (struct sockaddr *)&addr_in, sizeof(addr_in));
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    int pair2 = socket(PF_INET, SOCK_DGRAM, 0);
    addr_in.sin_port = htons(SIO_SELECT_SOCK_NOTIFY_PORT - 1);
    ret = bind(pair2, (struct sockaddr *)&addr_in, sizeof(addr_in));
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    slt->notify_pair[0] = pair1;
    slt->notify_pair[1] = pair2;

    return 0;
}

static inline
int sio_mplex_select_notify(struct sio_select *slt)
{
    char val = 1;

    struct sockaddr_in addr = { 0 };
    addr.sin_family = PF_INET;
    addr.sin_port = htons(SIO_SELECT_SOCK_NOTIFY_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    int ret = sendto(slt->notify_pair[1], &val, sizeof(char), 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

    return ret;
}

static inline
int sio_mplex_select_notify_read(struct sio_select *slt)
{
    char val = 0;
    struct sockaddr_in addr = { 0 };
    unsigned int l = sizeof(addr);
    int ret = recvfrom(slt->notify_pair[0], &val, sizeof(char), 0, (struct sockaddr *)&addr, &l);
    printf("notify_read val: %d, ret: %d\n", val, ret);

    return 0;
}

struct sio_mplex_ctx *sio_mplex_select_create(void)
{
    // static struct sio_mplex_ctx ctx = { 0 };
    struct sio_select *slt = malloc(sizeof(struct sio_select));
    SIO_COND_CHECK_RETURN_VAL(!slt, NULL);

    memset(slt, 0, sizeof(struct sio_select));

    int ret = sio_mplex_select_notifypair_create(slt);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, NULL,
        free(slt));

    sio_mutex_init(&slt->mutex);

    struct sio_event event = { .events = SIO_EVENTS_IN };
    sio_select_event_set(slt, slt->notify_pair[0], &event);

    return &slt->ctx;
}

int sio_mplex_select_ctl(struct sio_mplex_ctx *ctx, int op, sio_fd_t fd, struct sio_event *event)
{
    struct sio_select *slt = (struct sio_select *)ctx;
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(sio_select_check_out_bounds(slt, fd) == -1, -1,
        SIO_LOGE("socket descriptor %llu out of bounds\n", (sio_uint64_t)fd));

    if (op == SIO_EV_OPT_ADD || op == SIO_EV_OPT_MOD) {
        sio_select_event_set(slt, fd, event);
    } else if (op == SIO_EV_OPT_DEL) {
        sio_select_event_clr(slt, fd);
    }

    return 0;
}

int sio_mplex_select_wait(struct sio_mplex_ctx *ctx, struct sio_event *event, int count)
{
    struct sio_select *slt = (struct sio_select *)ctx;
    int resolved = sio_select_event(slt, event, count);
    SIO_COND_CHECK_RETURN_VAL(resolved > 0, resolved);

    sio_mutex_lock(&slt->mutex);
    struct sio_select_fin *in = &slt->fin;
    struct sio_select_fout *out = &tls_fout;
    memcpy(&out->fds, &in->fds, sizeof(struct sio_select_rwfds));

    int maxfd = in->maxfd; // for linux platform
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

    int ret = sio_mplex_select_notify(slt);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        SIO_LOGE("select notify write failed\n"));

    return ret;
}

int sio_mplex_select_destory(struct sio_mplex_ctx *ctx)
{
    free(ctx);
    return 0;
}