#include "sio_uring.h"
#include <string.h>
#include <stdlib.h>
#include <liburing.h>
#include "sio_common.h"
#include "sio_mplex_pri.h"
#include "sio_aioseq.h"

struct sio_uring
{
    struct io_uring uring;
    struct io_uring_params uparams;
};

#define sio_mplex_get_uring(ctx) ctx->fd.pfd

static inline
struct sio_aioseq *sio_aioseq_base_event(struct sio_event *event)
{
    char *ptr = sio_aiobuf_aioctx_ptr(event->buf.ptr);
    SIO_COND_CHECK_RETURN_VAL(ptr == NULL, NULL);

    memset(ptr, 0, sizeof(struct sio_aioseq));

    struct sio_aioseq *seq = (struct sio_aioseq *)ptr;
    seq->buf = event->buf.ptr;
    seq->len = event->buf.len;
    seq->ptr = event->pri;
    seq->events = event->events;

    return seq;
}

static inline
int sio_mplex_uring_accept(struct sio_uring *uring, sio_fd_t fd, struct sio_event *event)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(&uring->uring);

    io_uring_prep_multishot_accept(sqe, fd, NULL, NULL, 0);

    struct sio_aioseq *evseq = sio_aioseq_base_event(event);
    io_uring_sqe_set_data(sqe, evseq);

    int ret = io_uring_submit(&uring->uring);

    return ret;
}

static inline
int sio_mplex_uring_recv(struct sio_uring *uring, sio_fd_t fd, struct sio_event *event)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(&uring->uring);

    io_uring_prep_recv(sqe, fd, event->buf.ptr, event->buf.len, 0);

    struct sio_aioseq *evseq = sio_aioseq_base_event(event);
    io_uring_sqe_set_data(sqe, evseq);

    int ret = io_uring_submit(&uring->uring);
    
    return ret;
}

static inline
int sio_mplex_uring_send(struct sio_uring *uring, sio_fd_t fd, struct sio_event *event)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(&uring->uring);

    io_uring_prep_send(sqe, fd, event->buf.ptr, event->buf.len, 0);

    struct sio_aioseq *evseq = sio_aioseq_base_event(event);
    io_uring_sqe_set_data(sqe, evseq);

    int ret = io_uring_submit(&uring->uring);
    
    return ret;
}

struct sio_mplex_ctx *sio_mplex_uring_create(void)
{
    struct sio_mplex_ctx *ctx = malloc(sizeof(struct sio_mplex_ctx));
    SIO_COND_CHECK_RETURN_VAL(!ctx, NULL);

    memset(ctx, 0, sizeof(struct sio_mplex_ctx));

    struct sio_uring *uring = malloc(sizeof(struct sio_uring));
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!uring, NULL,
        free(ctx));

    memset(uring, 0, sizeof(struct sio_uring));

    int ret= io_uring_queue_init_params(1024, &uring->uring, &uring->uparams);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret != 0, NULL,
        free(uring),
        free(ctx));

    ctx->fd.pfd = uring;

    return ctx;
}

int sio_mplex_uring_ctl(struct sio_mplex_ctx *ctx, int op, sio_fd_t fd, struct sio_event *event)
{
    int ret = -1;
    if (op == SIO_EV_OPT_ADD || op == SIO_EV_OPT_MOD) {
        if (event->events & SIO_EVENTS_ASYNC_ACCEPT_RES) {
            ret = 0;
        } else if (event->events & SIO_EVENTS_ASYNC_ACCEPT_RES) {
            event->events |= ~SIO_EVENTS_ASYNC_ACCEPT;
            struct sio_uring *uring = sio_mplex_get_uring(ctx);
            ret = sio_mplex_uring_accept(uring, fd, event);
        } else if (event->events & SIO_EVENTS_ASYNC_READ) {
            struct sio_uring *uring = sio_mplex_get_uring(ctx);
            ret = sio_mplex_uring_recv(uring, fd, event);
        } else if (event->events & SIO_EVENTS_ASYNC_WRITE) {
            struct sio_uring *uring = sio_mplex_get_uring(ctx);
            ret = sio_mplex_uring_send(uring, fd, event);
        }
    }

    return ret;
}

int sio_mplex_uring_wait(struct sio_mplex_ctx *ctx, struct sio_event *event, int count)
{
    struct sio_uring *uring = sio_mplex_get_uring(ctx);
    struct io_uring_cqe *cqe;
    int ret = io_uring_wait_cqe(&uring->uring, &cqe);
    SIO_COND_CHECK_RETURN_VAL(ret != 0, -1);

    struct io_uring_cqe *cqes[count];
    ret = io_uring_peek_batch_cqe(&uring->uring, cqes, count);
    for (int i = 0; i < ret; i++) {
        struct sio_aioseq *evseq = (void *) (uintptr_t)cqes[i]->user_data;
        event[i].events = evseq->events;
        event[i].pri = evseq->ptr;
        event[i].res = cqes[i]->res;
        event[i].buf.ptr = evseq->buf;
        event[i].buf.len = evseq->len;
    }

    return ret;
}

int sio_mplex_uring_close(struct sio_mplex_ctx *ctx)
{
    return 0;
}

int sio_mplex_uring_destory(struct sio_mplex_ctx *ctx)
{
    return 0;
}