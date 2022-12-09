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

#define SIO_SERVER_MPTHRS_BITS 4
#define SIO_SERVER_MPTHRS_MAX ((1 << (SIO_SERVER_MPTHRS_BITS - 1)) - 1)

#define SIO_SERVER_WORKTHRS_BITS 8
#define SIO_SERVER_WORKTHRS_MAX ((1 << (SIO_SERVER_WORKTHRS_BITS - 1)) - 1)

struct sio_server_thread_model
{

    struct sio_mplex_thread *mthread[SIO_SERVER_MPTHRS_MAX];
    struct sio_thread *wthread[SIO_SERVER_WORKTHRS_MAX];
};

struct sio_server_mlb
{
    struct sio_server_thread_mask tmask;
    unsigned int round;
};

struct sio_server_attr
{
    int a;
};

struct sio_server
{
    struct sio_socket *sock;
    struct sio_server_thread_model tmodel;
    struct sio_server_mlb mlb;
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
void sio_server_thread_mask_check(struct sio_server_thread_mask *tmask)
{
    // limit mplex and work max threads 
    unsigned int mthrs = SIO_SERVER_MPTHRS_MAX & tmask->mthrs;
    unsigned int wthrs = SIO_SERVER_WORKTHRS_MAX & tmask->wthrs;

    unsigned int mod = wthrs % mthrs;
    wthrs = mod == 0 ? wthrs : wthrs - mod + mthrs;
    wthrs = wthrs <= SIO_SERVER_WORKTHRS_MAX ? wthrs : wthrs - mthrs;

    tmask->mthrs = mthrs;
    tmask->wthrs = wthrs;
}

#define SIO_SERVER_THREADS_CREATE(thread, ops, count, ret) \
    do { \
        for (int i = 0; i < count; i++) { \
            thread[i] = ops; \
            if (thread[i] == NULL) { \
                ret = 1; \
                break; \
            } \
        } \
    } while (0);

#define SIO_SERVER_THREADS_DESTORY(thread, count) \
    do { \
        for (int i = 0; i < count; i++) { \
            if (thread[i]) { \
                free(thread[i]); \
                thread[i] = NULL; \
            } else { \
                break; \
            } \
        } \
    } while (0);


static inline
int sio_server_threads_create(struct sio_server *serv, struct sio_server_thread_mask *tmask)
{
    sio_server_thread_mask_check(tmask);

    struct sio_server_thread_model *tmodel = &serv->tmodel;
    struct sio_mplex_thread **mthread = tmodel->mthread;
    struct sio_thread **wthread = tmodel->wthread;

#ifdef WIN32
    enum SIO_MPLEX_TYPE type = SIO_MPLEX_IOCP;
#else
    enum SIO_MPLEX_TYPE type = SIO_MPLEX_EPOLL;
#endif

    int ret = 0;
    SIO_SERVER_THREADS_CREATE(mthread, sio_mplex_thread_create(type), tmask->mthrs, ret);
    if (ret == -1) {
        SIO_SERVER_THREADS_DESTORY(mthread, SIO_SERVER_MPTHRS_MAX);
        return -1;
    }

    SIO_SERVER_THREADS_CREATE(wthread, 
        sio_thread_create(sio_work_thread_start_routine, serv), tmask->wthrs, ret);
    if (ret == -1) {
        SIO_SERVER_THREADS_DESTORY(wthread, SIO_SERVER_WORKTHRS_MAX);
        return -1;
    }

    struct sio_server_mlb *mlb = &serv->mlb;
    mlb->tmask = *tmask;

    return 0;
}

static inline
int sio_server_socket_mlb(struct sio_server *serv, struct sio_socket *sock)
{
    struct sio_server_mlb *mlb = &serv->mlb;
    struct sio_server_thread_mask *tmask = &mlb->tmask;
    struct sio_server_thread_model *tmodel = &serv->tmodel;
    struct sio_mplex_thread **mthread = tmodel->mthread;

    unsigned int round = mlb->round;
    unsigned int mthrs = tmask->mthrs;

    struct sio_mplex_thread *mthr = mthread[round % mthrs];
    SIO_COND_CHECK_RETURN_VAL(!mthr, -1);

    struct sio_mplex *mplex = sio_mplex_thread_mplex_ref(mthr);
    SIO_COND_CHECK_RETURN_VAL(!mplex, -1);

    int ret = sio_socket_mplex_bind(sock, mplex);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    ret = sio_socket_mplex(sock, SIO_EV_OPT_ADD, SIO_EVENTS_IN);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    round++;
    mlb->round = round;

    return 0;
}

static inline
struct sio_server *sio_server_create_imp(enum sio_socket_proto type, struct sio_server_thread_mask *tmask)
{
    struct sio_server *serv = malloc(sizeof(struct sio_server));
    SIO_COND_CHECK_RETURN_VAL(!serv, NULL);

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

    int ret = sio_server_threads_create(serv, tmask);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, NULL,
        sio_socket_destory(sock),
        free(serv));

    return serv;
}

struct sio_server *sio_server_create(enum sio_socket_proto type)
{
    struct sio_server_thread_mask tmask = {1, 2};
    return sio_server_create_imp(type, &tmask);
}

struct sio_server *sio_server_create2(enum sio_socket_proto type, struct sio_server_thread_mask tmask)
{
    return sio_server_create_imp(type, &tmask);
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
    ret = sio_socket_option(sock, SIO_SOCK_NONBLOCK, &opt);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    ret = sio_server_socket_mlb(serv, sock);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

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