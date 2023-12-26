#include "sio_server.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sio_common.h"
#include "sio_def.h"
#include "sio_socket.h"
#include "sio_mplex.h"
#include "sio_pmplex.h"
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

#ifdef WIN32
#define SIO_SYNC_IO     SIO_MPLEX_SELECT
#define SIO_ASYNC_IO    SIO_MPLEX_IOCP
#else
#define SIO_SYNC_IO     SIO_MPLEX_EPOLL
#define SIO_ASYNC_IO    SIO_MPLEX_URING
#endif

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
    struct sio_servops ops;
};

struct sio_server
{
    struct sio_socket *sock;
    struct sio_server_owner owner;
    struct sio_pmplex *pmplexs[SIO_SERVER_MAX_THREADS];
    struct sio_server_mlb mlb;
};

#define sio_socket_memptr char

struct sio_sockunion
{
    struct sio_server *serv;
    sio_socket_memptr sock[0];
};

sio_tls_t enum SIO_MPLEX_TYPE tls_mplex = SIO_SYNC_IO;

static inline
int sio_server_socket_mlb(struct sio_server *serv, struct sio_socket *sock);

static inline
struct sio_socket *sio_server_alloc_socket(struct sio_server *serv, int flag)
{
    int blocksize = sizeof(struct sio_sockunion) + sio_socket_struct_size();
    char *memptr = malloc(blocksize);
    SIO_COND_CHECK_RETURN_VAL(!memptr, NULL);

    memset(memptr, 0, blocksize);
    struct sio_sockunion *sockuni = (struct sio_sockunion *)memptr;
    sockuni->serv = serv;

    char *sockmem = sockuni->sock;
    struct sio_socket *sock;
    if (flag) {
        sock = sio_socket_dup2(serv->sock, sockmem);
    } else {
        sock = sio_socket_dup(serv->sock, sockmem);
    }

    return sock;
}

static inline
void sio_server_free_socket(struct sio_socket *sock)
{
    struct sio_sockunion *sockuni = SIO_CONTAINER_OF(sock, struct sio_sockunion, sock);
    free(sockuni);
}

static inline
struct sio_server *sio_server_socket_get_server(struct sio_socket *sock)
{
    struct sio_sockunion *sockuni = SIO_CONTAINER_OF(sock, struct sio_sockunion, sock);
    return sockuni->serv;
}

static inline
int sio_server_async_post_accept(struct sio_server *serv)
{
    struct sio_socket *sock = sio_server_alloc_socket(serv, tls_mplex == SIO_MPLEX_IOCP);
    SIO_COND_CHECK_RETURN_VAL(!sock, -1);

    return sio_socket_async_accept(serv->sock, sock);
}

static inline
int sio_server_newconnection_cb(struct sio_server *serv, struct sio_socket *sock)
{
    struct sio_server_owner *owner = &serv->owner;

    return owner->ops.newconnection == NULL ? 0 : owner->ops.newconnection(sock);
}

static inline
int sio_server_accpet_socket(struct sio_socket *serv)
{
    union sio_sockopt opt = { 0 };
    sio_socket_getopt(serv, SIO_SOCK_PRIVATE, &opt);
    struct sio_server *server = opt.private;
    SIO_COND_CHECK_RETURN_VAL(!server, -1);

    do {
        int ret = sio_socket_accept_has_pend(serv);
        SIO_COND_CHECK_BREAK(ret == SIO_ERRNO_AGAIN);
        SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

        struct sio_socket *sock = sio_server_alloc_socket(server, 0);
        SIO_COND_CHECK_RETURN_VAL(!sock, -1);

        ret = sio_socket_accept(serv, sock);
        SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
            sio_server_free_socket(sock));

        sio_server_newconnection_cb(server, sock);

        sio_server_socket_mlb(server, sock);

    } while (1);

    return 0;
}

static inline
int sio_server_async_accpet_socket(struct sio_socket *serv, struct sio_socket *sock)
{
    union sio_sockopt opt = { 0 };
    sio_socket_getopt(serv, SIO_SOCK_PRIVATE, &opt);
    struct sio_server *server = opt.private;
    SIO_COND_CHECK_RETURN_VAL(!server, -1);

    sio_server_async_post_accept(server);

    sio_server_socket_mlb(server, sock);

    sio_server_newconnection_cb(server, sock);

    return 0;
}

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
                sio_pmplex_destory(thread[i]); \
                thread[i] = NULL; \
            } \
        } \
    } while (0);


static inline
int sio_server_threads_create(struct sio_server *serv, unsigned char threads)
{
    struct sio_pmplex **pmplexs = serv->pmplexs;

    int ret = 0;
    SIO_SERVER_THREADS_CREATE(pmplexs, sio_pmplex_create(tls_mplex), threads, ret);
    if (ret == -1) {
        SIO_SERVER_THREADS_DESTORY(pmplexs, threads);
        return -1;
    }

    struct sio_server_mlb *mlb = &serv->mlb;
    mlb->threads = threads;

    return 0;
}

static inline
int sio_server_socket_mlb(struct sio_server *serv, struct sio_socket *sock)
{
    struct sio_pmplex **pmplexs = serv->pmplexs;
    struct sio_server_mlb *mlb = &serv->mlb;

    unsigned int round = mlb->round;
    unsigned int threads = mlb->threads;

    struct sio_pmplex *pmplex = pmplexs[round % threads];
    SIO_COND_CHECK_RETURN_VAL(!pmplex, -1);

    struct sio_mplex *mplex = sio_pmplex_mplex_ref(pmplex);
    SIO_COND_CHECK_RETURN_VAL(!mplex, -1);

    union sio_sockopt opt = {
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
struct sio_server *sio_server_create_imp(enum sio_sockprot prot, unsigned char threads)
{
    SIO_COND_CHECK_RETURN_VAL(threads == 0, NULL);

    struct sio_server *serv = malloc(sizeof(struct sio_server));
    SIO_COND_CHECK_RETURN_VAL(!serv, NULL);

    memset(serv, 0, sizeof(struct sio_server));

    struct sio_socket *sock = sio_socket_create(prot, NULL);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!sock, NULL,
        free(serv));

    union sio_sockopt opt = { 0 };
    opt.private = serv;
    sio_socket_setopt(sock, SIO_SOCK_PRIVATE, &opt);

    opt.ops.readable = sio_server_accpet_socket;
    opt.ops.acceptasync = sio_server_async_accpet_socket;
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
void sio_server_get_private(struct sio_server *serv, void **private)
{
    struct sio_server_owner *owner = &serv->owner;
    *private = owner->pri;
}

static inline
void sio_server_set_ops(struct sio_server *serv, struct sio_servops *ops)
{
    // serv->ops = *ops;
    struct sio_server_owner *owner = &serv->owner;
    owner->ops = *ops;
}

int sio_server_set_default(enum sio_servio mode)
{
    if (mode == SIO_SERV_NIO) {
        tls_mplex = SIO_SYNC_IO;
    } else if (mode == SIO_SERV_AIO){
        tls_mplex = SIO_ASYNC_IO;
    } else {
        return -1;
    }

    return 0;
}

struct sio_server *sio_server_create(enum sio_sockprot prot)
{
    return sio_server_create_imp(prot, 1);
}

struct sio_server *sio_server_create2(enum sio_sockprot prot, unsigned char threads)
{
    return sio_server_create_imp(prot, threads);
}

int sio_server_setopt(struct sio_server *serv, enum sio_servoptc cmd, union sio_servopt *opt)
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
    
    case SIO_SERV_SSL_CACERT:
    {
        union sio_sockopt opt2 = { .data = opt->data };
        sio_socket_setopt(serv->sock, SIO_SOCK_SSL_CACERT, &opt2);
        break;
    }
    case SIO_SERV_SSL_USERCERT:
    {
        union sio_sockopt opt2 = { .data = opt->data };
        sio_socket_setopt(serv->sock, SIO_SOCK_SSL_USERCERT, &opt2);
        break;
    }
    case SIO_SERV_SSL_USERKEY:
    {
        union sio_sockopt opt2 = { .data = opt->data };
        sio_socket_setopt(serv->sock, SIO_SOCK_SSL_USERKEY, &opt2);
        break;
    }
    case SIO_SERV_SSL_VERIFY_PEER:
    {
        union sio_sockopt opt2 = { .enable = opt->enable };
        sio_socket_setopt(serv->sock, SIO_SOCK_SSL_VERIFY_PEER, &opt2);
        break;
    }

    default:
        ret = -1;
        break;
    }

    return ret;
}

int sio_server_getopt(struct sio_server *serv, enum sio_servoptc cmd, union sio_servopt *opt)
{
    SIO_COND_CHECK_RETURN_VAL(!serv || !opt, -1);

    int ret = 0;
    switch (cmd) {
    case SIO_SERV_PRIVATE:
        sio_server_get_private(serv, &opt->private);
        break;

    default:
        ret = -1;
        break;
    }

    return ret;
}

int sio_server_listen(struct sio_server *serv, struct sio_sockaddr *addr)
{
    SIO_COND_CHECK_RETURN_VAL(!serv || !addr, -1);

    struct sio_socket *sock = serv->sock;
    int ret = sio_socket_listen(sock, addr);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    union sio_sockopt opt = { 0 };
    opt.nonblock = 1;
    ret = sio_socket_setopt(sock, SIO_SOCK_NONBLOCK, &opt);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    ret = sio_server_socket_mlb(serv, sock);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    SIO_COND_CHECK_CALLOPS(ret == 0 && tls_mplex == SIO_ASYNC_IO,
        sio_server_async_post_accept(serv));

    return 0;
}

struct sio_server *sio_server_socket_server(struct sio_socket *sock)
{
    return sio_server_socket_get_server(sock);
}

int sio_server_socket_reuse(struct sio_socket *sock)
{
    return -1;
}

void sio_server_socket_free(struct sio_socket *sock)
{
    sio_server_free_socket(sock);
}

int sio_server_shutdown(struct sio_server *serv)
{
    SIO_COND_CHECK_RETURN_VAL(!serv, -1);

    return sio_socket_shutdown(serv->sock, SIO_SOCK_SHUTRDWR);
}

int sio_server_destory(struct sio_server *serv)
{
    SIO_COND_CHECK_RETURN_VAL(!serv, -1);

    struct sio_pmplex **pmplexs = serv->pmplexs;
    SIO_SERVER_THREADS_DESTORY(pmplexs, SIO_SERVER_MAX_THREADS);

    sio_socket_destory(serv->sock);
    free(serv);

    return 0;
}