#include "sio_server.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sio_common.h"
#include "sio_socket.h"
#include "sio_mplex.h"
#include "sio_mplex_thread.h"
#include "sio_thread.h"
#include "sio_errno.h"
#include "sio_log.h"
// #include <windows.h>
// MemoryBarrier();

/* x86 */
#define smb_mb() __asm__ __volatile__("mfence":::"memory")

// arm64

#define dmb(opt)	asm volatile("dmb"#opt:::"memory")

// static inline
// int __attribute__((always_inline)) test(void)
// {
//     return 0;
// }

#define SIO_SERVER_MAX_THREADS 255

struct sio_server_mlb
{
    unsigned char threads;
    unsigned int round;
};

// struct sio_server_attr
// {
//     int a;
// };

struct sio_server_owner
{
    void *pri;
    struct sio_server_ops ops;
};

struct sio_server
{
    struct sio_socket *sock;
    struct sio_server_owner owner;
    struct sio_mplex_thread *mplths[SIO_SERVER_MAX_THREADS];
    struct sio_server_mlb mlb;
};

static int sio_socket_accpet(void *ptr, const char *data, int len);
void *sio_work_thread_start_routine(void *arg);

static struct sio_socket_ops g_serv_ops = 
{
    .read_cb = sio_socket_accpet
};

#define SIO_SERVER_THREADS_CREATE(thread, ops, count, ret) \
    do { \
        for (int i = 0; i < count; i++) { \
            thread[i] = ops; \
            if (thread[i] == NULL) { \
                ret = -1; \
                break; \
            } \
        } \
    } while (0);

#define SIO_SERVER_THREADS_DESTORY(thread, count) \
    do { \
        for (int i = 0; i < count; i++) { \
            if (thread[i]) { \
                sio_mplex_thread_destory(thread[i]); \
                thread[i] = NULL; \
            } \
        } \
    } while (0);


static inline
int sio_server_threads_create(struct sio_server *serv, unsigned char threads)
{
    struct sio_mplex_thread **mplths = serv->mplths;

#ifdef WIN32
    enum SIO_MPLEX_TYPE type = SIO_MPLEX_IOCP;
#else
    enum SIO_MPLEX_TYPE type = SIO_MPLEX_EPOLL;
#endif

    int ret = 0;
    SIO_SERVER_THREADS_CREATE(mplths, sio_mplex_thread_create(type), threads, ret);
    if (ret == -1) {
        SIO_SERVER_THREADS_DESTORY(mplths, threads);
        return -1;
    }

    struct sio_server_mlb *mlb = &serv->mlb;
    mlb->threads = threads;

    return 0;
}

static inline
int sio_server_socket_mlb(struct sio_server *serv, struct sio_socket *sock)
{
    struct sio_mplex_thread **mplths = serv->mplths;
    struct sio_server_mlb *mlb = &serv->mlb;

    unsigned int round = mlb->round;
    unsigned int threads = mlb->threads;

    struct sio_mplex_thread *mplth = mplths[round % threads];
    SIO_COND_CHECK_RETURN_VAL(!mplth, -1);

    struct sio_mplex *mplex = sio_mplex_thread_mplex_ref(mplth);
    SIO_COND_CHECK_RETURN_VAL(!mplex, -1);

    union sio_socket_opt opt = {
        .mplex = mplex
    };
    int ret = sio_socket_setopt(sock, SIO_SOCK_MPLEX, &opt);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    ret = sio_socket_mplex(sock, SIO_EV_OPT_ADD, SIO_EVENTS_IN);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    round++;
    mlb->round = round;

    return 0;
}

static inline
struct sio_server *sio_server_create_imp(enum sio_socket_proto type, unsigned char threads)
{
    SIO_COND_CHECK_RETURN_VAL(threads == 0, NULL);

    struct sio_server *serv = malloc(sizeof(struct sio_server));
    SIO_COND_CHECK_RETURN_VAL(!serv, NULL);

    memset(serv, 0, sizeof(struct sio_server));

    struct sio_socket *sock = sio_socket_create(type);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!sock, NULL,
        free(serv));

    union sio_socket_opt opt = { 0 };
    opt.private = serv;
    sio_socket_setopt(sock, SIO_SOCK_PRIVATE, &opt);
    opt.ops = g_serv_ops;
    sio_socket_setopt(sock, SIO_SOCK_OPS, &opt);

    serv->sock = sock;

    int ret = sio_server_threads_create(serv, threads);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, NULL,
        sio_socket_destory(sock),
        free(serv));

    return serv;
}

static inline
void sio_server_set_private(struct sio_server *serv, void *private)
{
    struct sio_server_owner *owner = &serv->owner;
    owner->pri = private;
}

static inline
void sio_server_set_ops(struct sio_server *serv, struct sio_server_ops *ops)
{
    // serv->ops = *ops;
    struct sio_server_owner *owner = &serv->owner;
    owner->ops = *ops;
}

struct sio_server *sio_server_create(enum sio_socket_proto type)
{
    return sio_server_create_imp(type, 1);
}

struct sio_server *sio_server_create2(enum sio_socket_proto type, unsigned char threads)
{
    return sio_server_create_imp(type, threads);
}

int sio_server_setopt(struct sio_server *serv, enum sio_server_optcmd cmd, union sio_server_opt *opt)
{
    SIO_COND_CHECK_RETURN_VAL(!serv, -1);
    SIO_COND_CHECK_RETURN_VAL(!opt, -1);

    int ret = 0;
    switch (cmd) {
    case SIO_SERV_PRIVATE:
        sio_server_set_private(serv, opt->private);
        break;
    case SIO_SERV_OPS:
        sio_server_set_ops(serv, &opt->ops);
        break;
    
    default:
        ret = -1;
        break;
    }

    return ret;
}

int sio_server_listen(struct sio_server *serv, struct sio_socket_addr *addr)
{
    SIO_COND_CHECK_RETURN_VAL(!serv || !addr, -1);

    struct sio_socket *sock = serv->sock;
    int ret = sio_socket_listen(sock, addr);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    union sio_socket_opt opt = { 0 };
    opt.nonblock = 1;
    ret = sio_socket_setopt(sock, SIO_SOCK_NONBLOCK, &opt);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    ret = sio_server_socket_mlb(serv, sock);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    return 0;
}

int sio_server_accept(struct sio_server *serv, struct sio_socket* sock)
{
    SIO_COND_CHECK_RETURN_VAL(!serv || !sock, -1);

    return sio_socket_accept(serv->sock, sock);
}

int sio_server_socket_mplex(struct sio_server *serv, struct sio_socket *sock)
{
    SIO_COND_CHECK_RETURN_VAL(!serv || !sock, -1);

    return sio_server_socket_mlb(serv, sock);
}

static inline
int sio_server_accept_cb(struct sio_server *serv)
{
    struct sio_server_owner *owner = &serv->owner;

    return owner->ops.accept_cb == NULL ? 0 : owner->ops.accept_cb(serv);
}

static inline
int sio_server_close_cb(struct sio_server *serv)
{
    struct sio_server_owner *owner = &serv->owner;
    return owner->ops.close_cb == NULL ? 0 : owner->ops.close_cb(serv);
}

static int sio_socket_accpet(void *ptr, const char *data, int len)
{
    struct sio_socket *sock_serv = ptr;
    struct sio_server *serv = sio_socket_private(sock_serv);
    SIO_COND_CHECK_RETURN_VAL(!serv, -1);

    do {
        int ret = sio_socket_accept_has_pend(serv->sock);
        SIO_COND_CHECK_BREAK(ret == SIO_ERRNO_AGAIN);
        SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
            sio_server_close_cb(serv));

        sio_server_accept_cb(serv);

    } while (1);

    return 0;
}

void *sio_work_thread_start_routine(void *arg)
{
    return NULL;
}

int sio_server_shutdown(struct sio_server *serv)
{
    SIO_COND_CHECK_RETURN_VAL(!serv, -1);

    return sio_socket_shutdown(serv->sock, SIO_SOCK_SHUTRDWR);
}

int sio_server_destory(struct sio_server *serv)
{
    SIO_COND_CHECK_RETURN_VAL(!serv, -1);

    struct sio_mplex_thread **mplths = serv->mplths;
    SIO_SERVER_THREADS_DESTORY(mplths, SIO_SERVER_MAX_THREADS);

    sio_socket_destory(serv->sock);
    free(serv);

    return 0;
}