#include "sio_select.h"
#include <string.h>
#include "sio_common.h"
#include "sio_mplex_pri.h"
#include "sio_select_pri.h"
#include "sio_event.h"
#include "sio_mutex.h"
#include "sio_log.h"

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

static sio_mutex g_mutex = SIO_MUTEX_INITIALIZER;
static struct sio_select_fin g_fin = { 0 };

#ifdef WIN32
__declspec(thread) struct sio_select_fout tls_fout= { 0 };
#else
__thread struct sio_select_fout tls_fout= { 0 };
#endif

static inline 
void sio_select_in_fds_set(struct sio_select_rwfds *rwfds, int fd, enum sio_events events)
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
void sio_select_in_event_set(struct sio_event *evs, int fd, struct sio_event *event)
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
void sio_select_in_set(struct sio_select_fin *in, int fd, struct sio_event *event)
{
    sio_select_in_event_set(in->evs, fd, event);
    sio_select_in_fds_set(&in->fds, fd, event->events);
}

static inline
void sio_select_in_clr(struct sio_select_fin *in, int fd)
{
    struct sio_event event = { 0 };
    sio_select_in_event_set(in->evs, fd, &event);
    sio_select_in_fds_set(&in->fds, fd, SIO_EVENTS_NONE);
}

static inline
void sio_select_check_modify_maxfd(struct sio_select_fin *in, int fd)
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
void sio_select_event_set(int fd, struct sio_event *event)
{
    sio_mutex_lock(g_mutex);
    struct sio_select_fin *in = &g_fin;
    sio_select_in_set(in, fd, event);
#ifndef WIN32
    sio_select_check_modify_maxfd(in, fd);
#endif
    sio_mutex_unlock(g_mutex);
}

static inline
void sio_select_event_clr(int fd)
{
    sio_mutex_lock(g_mutex);
    struct sio_select_fin *in = &g_fin;
    sio_select_in_clr(in, fd);
#ifndef WIN32
    sio_select_check_modify_maxfd(in, fd);
#endif
    sio_mutex_unlock(g_mutex);
}


#define sio_in_event_owner(fd) g_fin.evs[fd].owner

#ifdef WIN32

static inline
int sio_in_event_owner_index(fd)
{
    struct sio_select_fin *in = &g_fin;
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
        int fd = fdset->fd_array[fdset->fd_count - 1]; \
        int index = sio_in_event_owner_index(fd); \
        if (index == -1) { \
            fdset->fd_count--; \
            continue; \
        } \
        event[resolved].events = evtype; \
        event[resolved].owner = sio_in_event_owner(index); \
        resolved++; \
        fdset->fd_count--; \
    }

#endif

static inline
int sio_select_event(struct sio_event *event, int count)
{
    sio_mutex_lock(g_mutex);
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
    sio_out_event_get(rfds, SIO_EVENTS_IN);
    sio_out_event_get(wfds, SIO_EVENTS_OUT);
#endif
    sio_mutex_unlock(g_mutex);

    return resolved;
}

static inline
int sio_select_check_out_bounds(int fd)
{
#ifndef WIN32
    SIO_COND_CHECK_RETURN_VAL(fd >= SIO_FD_SETSIZE, -1);
#else
    sio_mutex_lock(g_mutex);
    struct sio_select_fin *in = &g_fin;
    sio_fd_set *rfds = &in->fds.rfds;
    sio_fd_set *wfds = &in->fds.wfds;

    int index = sio_in_event_owner_index(fd);
    if (index == -1 && (rfds->fd_count > SIO_FD_SETSIZE ||
        wfds->fd_count > SIO_FD_SETSIZE)) {
        return -1;
    }
    sio_mutex_unlock(g_mutex);
#endif

    return 0;
}

struct sio_mplex_ctx *sio_mplex_select_open(void)
{
#ifdef WIN32
    sio_mutex_init(g_mutex);
#endif
    static struct sio_mplex_ctx ctx = { 0 };
    return &ctx;
}

int sio_mplex_select_ctl(struct sio_mplex_ctx *ctx, int op, int fd, struct sio_event *event)
{
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(sio_select_check_out_bounds(fd) == -1, -1,
        SIO_LOGE("socket descriptor %d out of bounds\n", fd));

    if (op == SIO_EV_OPT_ADD || op == SIO_EV_OPT_MOD) {
        sio_select_event_set(fd, event);
    } else if (op == SIO_EV_OPT_DEL) {
        sio_select_event_clr(fd);
    }

    return 0;
}

int sio_mplex_select_wait(struct sio_mplex_ctx *ctx, struct sio_event *event, int count)
{
    int resolved = sio_select_event(event, count);
    SIO_COND_CHECK_RETURN_VAL(resolved > 0, resolved);

    sio_mutex_lock(g_mutex);
    struct sio_select_fin *in = &g_fin;
    struct sio_select_fout *out = &tls_fout;
    memcpy(&out->fds, &in->fds, sizeof(struct sio_select_rwfds));

    int maxfd = in->maxfd; // for linux platform
    out->pends = maxfd;  // for linux platform

    sio_fd_set *rfds = &out->fds.rfds;
    sio_fd_set *wfds = &out->fds.wfds;
    sio_mutex_unlock(g_mutex);

    int ret = select(maxfd + 1, (fd_set *)rfds, (fd_set *)wfds, NULL, NULL/*&timeout*/);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    resolved = sio_select_event(event, count);
    return resolved;
}

int sio_mplex_select_close(struct sio_mplex_ctx *ctx)
{
    return 0;
}
