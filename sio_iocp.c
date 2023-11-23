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
#include "sio_overlap.h"
#include "sio_log.h"

#define SIO_IOCP_EXIT_MAGIC 1234

#ifdef WIN32

#define sio_mplex_get_iocp(ctx) ctx->fd.pfd

static inline
void *sio_iocp_create()
{
    void *iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    SIO_COND_CHECK_RETURN_VAL(iocp == NULL, NULL);
    
    return iocp;
}

static inline
int sio_iocp_link_fd(void *iocp, sio_fd_t fd)
{
    void *ret = CreateIoCompletionPort((HANDLE)fd, iocp, 0, 0);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == NULL, -1,
        SIO_LOGE("iocp bind failed\n"));

    return 0;
}

static inline
int sio_iocp_exit_notify(void *iocp)
{
    return PostQueuedCompletionStatus(iocp, 0, SIO_IOCP_EXIT_MAGIC, NULL);
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

int sio_mplex_iocp_ctl(struct sio_mplex_ctx *ctx, int op, sio_fd_t fd, struct sio_event *event)
{
    int ret = 0;
    if (op == SIO_EV_OPT_ADD || op == SIO_EV_OPT_MOD) {
        if (event->events & SIO_EVENTS_IN) {
            void *iocp = sio_mplex_get_iocp(ctx);
            ret = sio_iocp_link_fd(iocp, fd);
        }
    } else {
        int ret = CancelIoEx((HANDLE)fd, NULL);
        if (ret > 0 || (ret == 0 && GetLastError() == ERROR_NOT_FOUND)) {
            ret = 0;
        } else {
            SIO_LOGI("iocp cancel io failed, ret: %d, err: %d\n", ret, GetLastError());
            ret = -1;
        }
    }

    return ret;
}

int sio_mplex_iocp_wait(struct sio_mplex_ctx *ctx, struct sio_event *event, int count)
{
    unsigned long recv = 0;
    ULONG_PTR key;
    struct sio_overlap *ovlp = NULL;

    void *iocp = sio_mplex_get_iocp(ctx);
    int ret = GetQueuedCompletionStatus(iocp, &recv, &key, (LPOVERLAPPED *)&ovlp, INFINITE);
    if (key == SIO_IOCP_EXIT_MAGIC) {
        sio_iocp_exit_notify(iocp);
        return -1;
    }
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
    event[0].pri = ovlp->ptr;
    event[0].buf.ptr = ovlp->wsabuf.buf;
    event[0].buf.len = recv;

    return 1;
}

int sio_mplex_iocp_close(struct sio_mplex_ctx *ctx)
{
    void *iocp = sio_mplex_get_iocp(ctx);
    return sio_iocp_exit_notify(iocp);
}

int sio_mplex_iocp_destory(struct sio_mplex_ctx *ctx)
{
    void *iocp = sio_mplex_get_iocp(ctx);
    int ret = CloseHandle(iocp);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        SIO_LOGE("iocp CloseHandle failed\n"));

    return 0;
}

#else

struct sio_mplex_ctx *sio_mplex_iocp_create(void)
{
    return NULL;
}

int sio_mplex_iocp_ctl(struct sio_mplex_ctx *ctx, int op, sio_fd_t fd, struct sio_event *event)
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