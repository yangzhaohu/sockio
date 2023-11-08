#include "sio_iocp.h"
#include <string.h>
#include <stdlib.h>
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <mswsock.h>
#endif
#include "sio_common.h"
#include "sio_mplex_pri.h"
#include "sio_event.h"
#include "sio_log.h"

#ifdef WIN32

#define sio_mplex_get_iocp(ctx) ctx->fd.pfd

struct sio_overlapped
{
    OVERLAPPED overlap;
    WSABUF wsabuf;
    void *ptr;
    int fd;
    unsigned int events;
    unsigned long olflags;
};

// __declspec(thread) char tls_accept_addr_buf[2 * (sizeof(struct sockaddr_in) + 16)] = { 0 };

static inline
struct sio_overlapped *sio_iocp_overlapped_bufref(struct sio_event *event)
{
    char *ptr = event->buf.ptr;
    int len = event->buf.len;
    len -= sizeof(struct sio_overlapped);
    SIO_COND_CHECK_RETURN_VAL(ptr == NULL || len <= 0, NULL);
    ptr += len;
    event->buf.len = len;
    memset(ptr, 0, sizeof(struct sio_overlapped));
    
    return (struct sio_overlapped *)ptr;
}

static inline
void sio_iocp_overlapped_link(struct sio_overlapped *ovlp, struct sio_event *event)
{
    ovlp->wsabuf.buf = event->buf.ptr;
    ovlp->wsabuf.len = event->buf.len;
    ovlp->ptr = event->owner.pri;
    ovlp->fd = event->owner.fd;
    ovlp->events = event->events;
}

// static inline
// struct sio_overlapped *sio_iocp_overlapped_alloc()
// {
//     struct sio_overlapped *ovlp = malloc(sizeof(struct sio_overlapped));
//     SIO_COND_CHECK_RETURN_VAL(!ovlp, NULL);

//     memset(ovlp, 0, sizeof(struct sio_overlapped));

//     return ovlp;
// }

static inline
void *sio_iocp_create()
{
    void *iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    SIO_COND_CHECK_RETURN_VAL(iocp == NULL, NULL);
    
    return iocp;
}

static inline
int sio_iocp_link_fd(void *iocp, int fd)
{
    void *ret = CreateIoCompletionPort((HANDLE)fd, iocp, 0, 0);
    SIO_COND_CHECK_RETURN_VAL(ret == NULL, -1);

    return 0;
}

static inline
int sio_iocp_post_accept(struct sio_event *event, int fd)
{
    struct sio_overlapped *ovlp = sio_iocp_overlapped_bufref(event);
    SIO_COND_CHECK_RETURN_VAL(ovlp == NULL, -1);

    sio_iocp_overlapped_link(ovlp, event);

    unsigned long recvsize = 0;
    static char addrbuf[2 * (sizeof(struct sockaddr_in) + 16)] = { 0 };
    int ret = AcceptEx(ovlp->fd, fd, addrbuf, 0, 
        sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16, &recvsize, (LPOVERLAPPED)ovlp);

    SIO_COND_CHECK_RETURN_VAL(ret == 0 && WSAGetLastError() != ERROR_IO_PENDING, -1);

    return 0;
}

// static inline
// int sio_iocp_post_recv_imp(struct sio_overlapped *ovlp)
// {
//     int ret = WSARecv(ovlp->fd, &ovlp->wsabuf, 1, NULL, &ovlp->olflags, &ovlp->overlap, NULL);
//     SIO_COND_CHECK_RETURN_VAL(ret == -1 && WSAGetLastError() != ERROR_IO_PENDING, -1);

//     return 0;
// }

static inline
int sio_iocp_post_recv(struct sio_event *event)
{
    struct sio_overlapped *ovlp = sio_iocp_overlapped_bufref(event);
    SIO_COND_CHECK_RETURN_VAL(ovlp == NULL, -1);

    sio_iocp_overlapped_link(ovlp, event);

    int ret = WSARecv(ovlp->fd, &ovlp->wsabuf, 1, NULL, &ovlp->olflags, &ovlp->overlap, NULL);

    SIO_COND_CHECK_RETURN_VAL(ret == -1 && WSAGetLastError() != ERROR_IO_PENDING, -1);

    return 0;
}

struct sio_mplex_ctx *sio_mplex_iocp_create(void)
{
    struct sio_mplex_ctx *ctx = malloc(sizeof(struct sio_mplex_ctx));
    SIO_COND_CHECK_RETURN_VAL(!ctx, NULL);

    memset(ctx, 0, sizeof(struct sio_mplex_ctx));

    void *iocp = sio_iocp_create();
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!iocp, NULL,
        free(ctx));

    ctx->fd.pfd = iocp;
    return ctx;
}

int sio_mplex_iocp_ctl(struct sio_mplex_ctx *ctx, int op, int fd, struct sio_event *event)
{
    int ret = 0;
    if (event->events & SIO_EVENTS_ASYNC_READ) {
        ret = sio_iocp_post_recv(event);
    }
    if (event->events & SIO_EVENTS_ASYNC_ACCEPT) {
        ret = sio_iocp_post_accept(event, fd);
    }
    if (event->events & SIO_EVENTS_IN) {
        void *iocp = sio_mplex_get_iocp(ctx);
        sio_iocp_link_fd(iocp, event->owner.fd);
    }

    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    return 0;
}

int sio_mplex_iocp_wait(struct sio_mplex_ctx *ctx, struct sio_event *event, int count)
{
    unsigned long recv = 0;
    ULONG_PTR key;
    struct sio_overlapped *ovlp = NULL;

    void *iocp = sio_mplex_get_iocp(ctx);
    int ret = GetQueuedCompletionStatus(iocp, &recv, &key, (LPOVERLAPPED *)&ovlp, INFINITE);
    if (ret == FALSE) {
        int err = GetLastError();
        // SIO_LOGE("err: %d\n", err);
        SIO_COND_CHECK_RETURN_VAL(ovlp == NULL ||
                                  err != ERROR_CONNECTION_ABORTED, -1);
    }

    // unsigned long rmcnt = 0;
    // struct _OVERLAPPED_ENTRY ovlps[count];
    // memset(ovlps, 0, sizeof(struct _OVERLAPPED_ENTRY) * count);
    // ret = GetQueuedCompletionStatusEx(iocp, ovlps, count, &rmcnt, INFINITE, TRUE);

    event[0].events = ovlp->events;
    event[0].owner.pri = ovlp->ptr;
    event[0].buf.ptr = ovlp->wsabuf.buf;
    event[0].buf.len = recv;

    return 1;
}

int sio_mplex_iocp_close(struct sio_mplex_ctx *ctx)
{
    return -1;
}

int sio_mplex_iocp_destory(struct sio_mplex_ctx *ctx)
{
    return -1;
}

#else

struct sio_mplex_ctx *sio_mplex_iocp_create(void)
{
    return NULL;
}

int sio_mplex_iocp_ctl(struct sio_mplex_ctx *ctx, int op, int fd, struct sio_event *event)
{
    return -1;
}

int sio_mplex_iocp_wait(struct sio_mplex_ctx *ctx, struct sio_event *event, int count)
{
    return -1;
}

int sio_mplex_iocp_close(struct sio_mplex_ctx *ctx)
{
    return -1;
}

int sio_mplex_iocp_destory(struct sio_mplex_ctx *ctx)
{
    return -1;
}

#endif