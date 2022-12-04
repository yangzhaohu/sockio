#include "sio_server.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sio_common.h"
#include "sio_socket.h"
#include "sio_mplex.h"
#include "sio_mplex_thread.h"
#include "sio_thread.h"
// #include <windows.h>
// MemoryBarrier();

/* x86 */
#define smb_mb() __asm__ __volatile__("mfence":::"memory")

// arm64

#define dmb(opt)	asm volatile("dmb " #opt : : : "memory")

// static inline
// int __attribute__((always_inline)) test(void)
// {
//     return 0;
// }

struct sio_server_thread_model
{
    struct sio_server_thread_count thrc;
    struct sio_mplex_thread **mpthr;
    struct sio_thread **workthr;
};

struct sio_server_attr
{
    int a;
};

struct sio_server
{
    struct sio_socket *sock;
    struct sio_server_thread_model thrm;
};

static int sio_socket_accpet(void *ptr, const char *data, int len);
static int sio_socket_readable(void *ptr, const char *data, int len);
static int sio_socket_writeable(void *ptr, const char *data, int len);
void *sio_work_thread_start_routine(void *arg);

static struct sio_io_ops g_serv_ops = 
{
    .read_cb = sio_socket_accpet
};

static struct sio_io_ops g_sock_ops = 
{
    .read_cb = sio_socket_readable,
    .write_cb = sio_socket_writeable
};

static inline
int sio_server_thread_model_create(struct sio_server_thread_model *thrm, struct sio_server_thread_count *thrc)
{
    SIO_COND_CHECK_RETURN_VAL(thrc->mplex_count == 0 || thrc->work_multi == 0, -1);
    thrm->thrc = *thrc;

    int count = thrm->thrc.mplex_count;
    thrm->mpthr = malloc(sizeof(struct sio_mplex_thread *) * count);
    SIO_COND_CHECK_RETURN_VAL(!thrm->mpthr, -1);
    for (int i = 0; i < count; i++) {
        struct sio_mplex_thread *mpthr = sio_mplex_thread_create(SIO_MPLEX_EPOLL);
        SIO_COND_CHECK_RETURN_VAL(!mpthr, -1);
        thrm->mpthr[i] = mpthr;
    }

    count = thrm->thrc.mplex_count * thrc->work_multi;
    thrm->workthr = malloc(sizeof(struct sio_thread *) * count);
    for (int i = 0; i < count; i++) {
        struct sio_thread *thr = sio_thread_create(sio_work_thread_start_routine, NULL);
        SIO_COND_CHECK_RETURN_VAL(!thr, -1);
        thrm->workthr[i] = thr;
    }
    
    return 0;
}

static inline
int sio_server_thread_model_destory(struct sio_server *serv)
{
    return 0;
}

struct sio_server *sio_server_create(enum sio_socket_proto type, struct sio_server_thread_count *thrc)
{
    struct sio_server *serv = malloc(sizeof(struct sio_server));
    SIO_COND_CHECK_RETURN_VAL(!serv, NULL);
    //test();
    smb_mb();
    memset(serv, 0, sizeof(struct sio_server));

    struct sio_socket *sock = sio_socket_create(type);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!sock, NULL,
        free(serv));

    union sio_socket_opt opt = { 0 };
    opt.private = serv;
    sio_socket_option(sock, SIO_SOCK_PRIVATE, &opt);
    opt.ops = g_serv_ops;
    sio_socket_option(sock, SIO_SOCK_OPS, &opt);
    serv->sock = sock;

    return serv;
}

int sio_server_advance(struct sio_server *serv, enum sio_server_advcmd cmd, union sio_server_adv *adv)
{
    return 0;
}

int sio_server_listen(struct sio_server *serv, struct sio_socket_addr *addr)
{
    SIO_COND_CHECK_RETURN_VAL(!serv || !addr, -1);

    struct sio_socket *sock = serv->sock;
    int ret = sio_socket_listen(sock, addr);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    union sio_socket_opt opt = { 0 };
    opt.nonblock = 1;
    sio_socket_option(sock, SIO_SOCK_NONBLOCK, &opt);

    // struct sio_mplex *mplex = sio_server_get_mplex(serv->mpt, SIO_MPLEX_GROUP_ACCEPT);
    // SIO_COND_CHECK_RETURN_VAL(mplex == NULL, -1);

    // sio_socket_mplex_bind(sock, mplex);
    // sio_socket_mplex(sock, SIO_EV_OPT_ADD, SIO_EVENTS_IN);

    return 0;
}

static int sio_socket_accpet(void *ptr, const char *data, int len)
{
    struct sio_socket *sock_serv = ptr;
    struct sio_server *serv = sio_socket_private(sock_serv);
    SIO_COND_CHECK_RETURN_VAL(!serv, -1);

    struct sio_socket *sock = sio_socket_create(SIO_SOCK_TCP);
    SIO_COND_CHECK_RETURN_VAL(!sock, -1);

    int ret = sio_socket_accept(sock_serv, sock);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    union sio_socket_opt opt = { 0 };
    opt.ops = g_sock_ops;
    sio_socket_option(sock, SIO_SOCK_OPS, &opt);
    opt.nonblock = 1;
    sio_socket_option(sock, SIO_SOCK_NONBLOCK, &opt);

    // struct sio_mplex *mplex = sio_server_get_mplex(serv->mpt, SIO_MPLEX_GROUP_BUTT);
    // SIO_COND_CHECK_RETURN_VAL(mplex == NULL, -1);

    // sio_socket_mplex_bind(sock, mplex);
    // sio_socket_mplex(sock, SIO_EV_OPT_ADD, SIO_EVENTS_IN);

    return 0;
}

static int sio_socket_readable(void *ptr, const char *data, int len)
{
    if (len == 0) {
        printf("close\n");
        return 0;
    }
    printf("recv %d: %s\n", len, data);
    return 0;
}

static int sio_socket_writeable(void *ptr, const char *data, int len)
{
    printf("write\n");
    return 0;
}

void *sio_work_thread_start_routine(void *arg)
{
    return NULL;
}