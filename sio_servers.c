#include "sio_servers.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sio_common.h"
#include "sio_server.h"
#include "sio_thread.h"
#include "sio_queue.h"

#define SIO_SERVERS_MAX_THREADS 255

#define SIO_SERVER_THREADS_RATIO 16


struct sio_servers_owner
{
    struct sio_servers_ops ops;
};

struct sio_servers_sockpool
{
    unsigned char size;
    unsigned char index;
    struct sio_queue **sks;
};

struct sio_servers_taskpool
{
    unsigned char size;
    struct sio_thread **threads;
    struct sio_queue **tks;
};

struct sio_servers
{
    struct sio_server *serv;
    struct sio_servers_owner owner;

    struct sio_servers_sockpool skpool;
    struct sio_servers_taskpool tkpool;
};


#define SIO_SERVERS_THREADS_CREATE(thread, ops, count, ret) \
    do { \
        for (int i = 0; i < count; i++) { \
            thread[i] = ops; \
            if (thread[i] == NULL) { \
                ret = -1; \
                break; \
            } \
        } \
    } while (0);

#define SIO_SERVERS_THREADS_START(thread, count, ret) \
    do { \
        for (int i = 0; i < count; i++) { \
            if (thread[i] != NULL) { \
                ret = sio_thread_start(thread[i]); \
                if (ret == -1) { \
                    break; \
                } \
            } \
        } \
    } while (0);

#define SIO_SERVERS_THREADS_DESTORY(thread, count) \
    do { \
        for (int i = 0; i < count; i++) { \
            if (thread[i]) { \
                sio_thread_destory(thread[i]); \
                thread[i] = NULL; \
            } \
        } \
    } while (0);

static int sio_socket_readable(struct sio_socket *sock, const char *data, int len)
{
    if (len == 0) {
        printf("client socket close\n");
        return 0;
    }

    return 0;
}

static int sio_socket_writeable(struct sio_socket *sock, const char *data, int len)
{
    return 0;
}

static int sio_server_newconn(struct sio_server *serv)
{
    struct sio_socket *sock = sio_socket_create(SIO_SOCK_TCP);
    union sio_socket_opt opt = {
        .ops.read = sio_socket_readable,
        .ops.write = sio_socket_writeable
    };
    sio_socket_setopt(sock, SIO_SOCK_OPS, &opt);

    int ret = sio_server_accept(serv, sock);
    if(ret == -1) {
        sio_socket_destory(sock);
        return ret;
    }

    opt.nonblock = 1;
    sio_socket_setopt(sock, SIO_SOCK_NONBLOCK, &opt);

    union sio_server_opt opts = { 0 };
    sio_server_getopt(serv, SIO_SERV_PRIVATE, &opts);
    struct sio_servers *servs = opts.private;
    if (servs) {
        struct sio_servers_owner *owner = &servs->owner;
        if (owner->ops.new_conn) {
            owner->ops.new_conn(sock);
        }
    }

    sio_server_socket_mplex(serv, sock);

    return ret;
}

void *sio_servers_thread_start_routine(void *arg)
{
    return NULL;
}

static inline
struct sio_server *sio_servers_create_server(enum sio_socket_proto type, unsigned char threads)
{
    struct sio_server *serv = sio_server_create2(type, threads);
    SIO_COND_CHECK_RETURN_VAL(!serv, NULL);

    union sio_server_opt ops = {
        .ops.accept = sio_server_newconn
    };
    sio_server_setopt(serv, SIO_SERV_OPS, &ops);

    return serv;
}

/*
* skpool init
*/

static inline
int sio_servers_skpool_queue_init(struct sio_servers_sockpool *skpool, int size)
{
    skpool->sks = malloc(sizeof(struct sio_queue *) * size);
    SIO_COND_CHECK_RETURN_VAL(skpool->sks == NULL, -1);

    for (int i = 0; i < size; i++) {
        skpool->sks[i] = sio_queue_create();
    }

    return 0;
}

static inline
int sio_servers_skpool_queue_uninit(struct sio_servers_sockpool *skpool)
{
    for (int i = 0; i < skpool->size; i++) {
        struct sio_queue *sk = skpool->sks[i];
        SIO_COND_CHECK_CALLOPS(sk != NULL,
            sio_queue_destory(sk),
            skpool->sks[i] = NULL);
    }

    free(skpool->sks);
    skpool->sks = NULL;

    return 0;
}

static inline
int sio_servers_skpool_init(struct sio_servers *servs, int size)
{
    struct sio_servers_sockpool *skpool = &servs->skpool;
    skpool->size = size;

    int ret = sio_servers_skpool_queue_init(skpool, size);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    return 0;
}

static inline
int sio_servers_skpool_uninit(struct sio_servers *servs)
{
    struct sio_servers_sockpool *skpool = &servs->skpool;

    sio_servers_skpool_queue_uninit(skpool);

    return 0;
}

/*
* tkpool init
*/

static inline
int sio_servers_tkpool_thread_create(struct sio_servers_taskpool *tkpool, int size)
{
    struct sio_thread **threads = tkpool->threads;
    struct sio_servers *servs = SIO_CONTAINER_OF(tkpool, struct sio_servers, tkpool);

    int ret = 0;
    SIO_SERVERS_THREADS_CREATE(threads, sio_thread_create(sio_servers_thread_start_routine, servs), size, ret);
    if (ret == -1) {
        SIO_SERVERS_THREADS_DESTORY(threads, size);
        return -1;
    }

    SIO_SERVERS_THREADS_START(threads, size, ret);
    if (ret == -1) {
        SIO_SERVERS_THREADS_DESTORY(threads, size);
        return -1;
    }

    return 0;
}

static inline
int sio_servers_tkpool_thread_destory(struct sio_servers_taskpool *tkpool, int size)
{
    struct sio_thread **threads = tkpool->threads;
    SIO_SERVERS_THREADS_DESTORY(threads, size);

    return 0;
}

static inline
int sio_servers_tkpool_thread_init(struct sio_servers_taskpool *tkpool, int size)
{
    tkpool->threads = malloc(sizeof(struct sio_thread *) * size);
    SIO_COND_CHECK_RETURN_VAL(tkpool->threads == NULL, -1);

    int ret = sio_servers_tkpool_thread_create(tkpool, size);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    return 0;
}

static inline
int sio_servers_tkpool_thread_uninit(struct sio_servers_taskpool *tkpool)
{
    sio_servers_tkpool_thread_destory(tkpool, tkpool->size);
    free(tkpool->threads);
    tkpool->threads = NULL;

    return 0;
}

static inline
int sio_servers_tkpool_queue_init(struct sio_servers_taskpool *tkpool, int size)
{
    tkpool->tks = malloc(sizeof(struct sio_queue *) * size);
    SIO_COND_CHECK_RETURN_VAL(tkpool->tks == NULL, -1);

    for (int i = 0; i < size; i++) {
        tkpool->tks[i] = sio_queue_create();
    }

    return 0;
}

static inline
int sio_servers_tkpool_queue_uninit(struct sio_servers_taskpool *tkpool)
{
    for (int i = 0; i < tkpool->size; i++) {
        struct sio_queue *tk = tkpool->tks[i];
        SIO_COND_CHECK_CALLOPS(tk != NULL, 
            sio_queue_destory(tk),
            tkpool->tks[i] = NULL);
    }

    free(tkpool->tks);
    tkpool->tks = NULL;

    return 0;
}

static inline
int sio_servers_tkpool_init(struct sio_servers *servs, int size)
{
    struct sio_servers_taskpool *tkpool = &servs->tkpool;
    tkpool->size = size;

    int ret = sio_servers_tkpool_thread_init(tkpool, size);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    ret = sio_servers_tkpool_queue_init(tkpool, size);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        sio_servers_tkpool_thread_uninit(tkpool));

    return 0;
}

static inline
int sio_servers_tkpool_uninit(struct sio_servers *servs)
{
    struct sio_servers_taskpool *tkpool = &servs->tkpool;

    sio_servers_tkpool_thread_uninit(tkpool);
    sio_servers_tkpool_queue_uninit(tkpool);

    return 0;
}

static inline
struct sio_servers *sio_servers_create_imp(enum sio_socket_proto type, unsigned char servthrs, unsigned char workthrs)
{
    struct sio_servers *servs = malloc(sizeof(struct sio_servers));
    SIO_COND_CHECK_RETURN_VAL(!servs, NULL);

    memset(servs, 0, sizeof(struct sio_servers));

    struct sio_server *serv = sio_servers_create_server(type, servthrs);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!serv, NULL,
        free(servs));

    union sio_server_opt ops = {
        .private = servs
    };
    sio_server_setopt(serv, SIO_SERV_PRIVATE, &ops);

    servs->serv = serv;

    int ret = sio_servers_skpool_init(servs, servthrs);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, NULL,
        sio_server_destory(serv),
        free(servs));

    ret = sio_servers_tkpool_init(servs, workthrs);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, NULL,
        sio_server_destory(serv),
        sio_servers_skpool_uninit(servs),
        free(servs));

    return servs;
}

static inline
void sio_servers_set_ops(struct sio_servers *servs, struct sio_servers_ops *ops)
{
    struct sio_servers_owner *owner = &servs->owner;
    owner->ops = *ops;
}

struct sio_servers *sio_servers_create(enum sio_socket_proto type)
{
    return sio_servers_create_imp(type, 1, 1);
}

struct sio_servers *sio_servers_create2(enum sio_socket_proto type, unsigned char threads)
{
    SIO_COND_CHECK_RETURN_VAL(threads == 0, NULL);

    unsigned char servthrs = threads % SIO_SERVER_THREADS_RATIO;
    servthrs = servthrs == 0 ? 1 : servthrs;
    unsigned char workthrs = threads - servthrs;
    workthrs = workthrs == 0 ? 1 : workthrs;
    return sio_servers_create_imp(type, servthrs, workthrs);
}

int sio_servers_setopt(struct sio_servers *servs, enum sio_servers_optcmd cmd, union sio_servers_opt *opt)
{
    SIO_COND_CHECK_RETURN_VAL(!servs || !opt, -1);

    int ret = 0;
    switch (cmd) {
    case SIO_SERVS_OPS:
        sio_servers_set_ops(servs, &opt->ops);
        break;

    default:
        ret = -1;
        break;
    }

    return ret;
}

int sio_servers_listen(struct sio_servers *servs, struct sio_socket_addr *addr)
{
    SIO_COND_CHECK_RETURN_VAL(!servs || !addr, -1);

    return sio_server_listen(servs->serv, addr);
}

int sio_servers_destory(struct sio_servers *servs)
{
    SIO_COND_CHECK_RETURN_VAL(!servs, -1);

    int ret = sio_server_destory(servs->serv);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    sio_servers_skpool_uninit(servs);
    sio_servers_tkpool_uninit(servs);

    free(servs);

    return 0;
}