#include "sio_servflow.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "sio_common.h"
#include "sio_server.h"
#include "sio_thread.h"
#include "sio_taskfifo.h"
#include "sio_atomic.h"

struct sio_sockflow
{
    struct sio_servflow *servflow;
    void *pri;
};

struct sio_sockflow_data
{
    struct sio_sockflow *sockflow;
    char data[512];
    int len;
};

struct sio_servflow_owner
{
    struct sio_servflow_ops ops;
};

struct sio_servflow_mlb
{
    unsigned char index;
    unsigned char flow;
};

struct sio_servflow_taskflow
{
    struct sio_taskfifo *tf;
    struct sio_servflow_mlb mlb;
};

struct sio_servflow
{
    struct sio_server *serv;
    struct sio_servflow_owner owner;

    struct sio_servflow_taskflow tflow;
};

// #ifdef WIN32
// __declspec(thread) int tls_tkpool_index = -1;
// #else
// __thread int tls_tkpool_index = -1;
// #endif

// static inline
// int sio_servflow_tkpool_getindex(struct sio_servflow_taskpool *tkpool)
// {
//     if (tls_tkpool_index == -1) {
//         tls_tkpool_index = sio_int_fetch_and_add(&tkpool->index, 1);
//     }

//     return tls_tkpool_index;
// }

static int sio_socket_readable(struct sio_socket *sock)
{
    union sio_socket_opt opt = { 0 };
    sio_socket_getopt(sock, SIO_SOCK_PRIVATE, &opt);

    struct sio_sockflow *sockflow = opt.private;
    struct sio_servflow *servflow = sockflow->servflow;

    struct sio_servflow_owner *owner = &servflow->owner;
    SIO_COND_CHECK_RETURN_VAL(!owner->ops.flow_data, -1);

    // char data[512] = { 0 };
    struct sio_sockflow_data *data = malloc(sizeof(struct sio_sockflow_data));
    memset(data, 0, sizeof(struct sio_sockflow_data));
    int len = sio_socket_read(sock, data->data, 512);
    if (len > 0) {
        data->sockflow = sockflow;
        data->len = len;
        struct sio_taskfifo *tf = servflow->tflow.tf;
        sio_taskfifo_in(tf, 0, &data, 1);
    } else {
        free(data);
    }

    return len;
}

static void *sio_sockflow_dataproc(void *handle, void *data)
{
    struct sio_servflow *servflow = handle;

    struct sio_sockflow_data *skdata = (struct sio_sockflow_data *)data;
    struct sio_servflow_owner *owner = &servflow->owner;
    if (owner->ops.flow_data) {
        owner->ops.flow_data(skdata->sockflow, skdata->data, skdata->len);
    }
    free(skdata);

    return NULL;
}

static int sio_socket_writeable(struct sio_socket *sock)
{
    return 0;
}

static int sio_socket_closeable(struct sio_socket *sock)
{
    union sio_socket_opt opt = { 0 };
    sio_socket_getopt(sock, SIO_SOCK_PRIVATE, &opt);

    struct sio_sockflow *sockflow = opt.private;
    sio_socket_close(sock);

    if (sockflow) {
        free(sockflow);
    }

    return 0;
}

static inline
void *sio_sockflow_sock_mem()
{
    int size = sizeof(struct sio_sockflow) + sio_socket_struct_size();
    void *mem = malloc(size);
    SIO_COND_CHECK_RETURN_VAL(!mem, NULL);

    memset(mem, 0, size);

    return mem;
}

static inline
struct sio_sockflow *sio_sockflow_setup(struct sio_servflow *servflow)
{
    void *mem = sio_sockflow_sock_mem();
    SIO_COND_CHECK_RETURN_VAL(!mem, NULL);

    char *smem = mem + sizeof(struct sio_sockflow);
    struct sio_socket *sock = sio_socket_create(SIO_SOCK_TCP, smem);
    union sio_socket_opt opt = {
        .ops.readable = sio_socket_readable,
        .ops.writeable = sio_socket_writeable,
        .ops.closeable = sio_socket_closeable
    };
    sio_socket_setopt(sock, SIO_SOCK_OPS, &opt);

    int ret = sio_server_accept(servflow->serv, sock);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, NULL,
        free(mem));

    opt.nonblock = 1;
    sio_socket_setopt(sock, SIO_SOCK_NONBLOCK, &opt);

    struct sio_sockflow *sockflow = (struct sio_sockflow *)mem;
    sockflow->servflow = servflow;
    opt.private = sockflow;
    sio_socket_setopt(sock, SIO_SOCK_PRIVATE, &opt);

    return sockflow;
}

static int sio_server_newconn(struct sio_server *serv)
{
    union sio_server_opt opt = { 0 };
    sio_server_getopt(serv, SIO_SERV_PRIVATE, &opt);
    struct sio_servflow *servflow = opt.private;

    struct sio_sockflow *sockflow = sio_sockflow_setup(servflow);
    if (sockflow) {
        struct sio_servflow_owner *owner = &servflow->owner;
        if (owner->ops.flow_new) {
            owner->ops.flow_new(sockflow);
        }
    }

    struct sio_socket *sock = (struct sio_socket *)((char *)sockflow + sizeof(struct sio_sockflow));
    sio_server_socket_mplex(serv, sock);

    return 0;
}

static inline
enum sio_socket_proto sio_servflow_sockproto(enum sio_servflow_proto type)
{
    enum sio_socket_proto proto = SIO_SOCK_TCP;
    switch (type) {
    case SIO_SERVFLOW_TCP:
        proto = SIO_SOCK_TCP;
        break;

    default:
        break;
    }

    return proto;
}

static inline
struct sio_server *sio_servflow_server_create(enum sio_servflow_proto type, unsigned char threads)
{
    enum sio_socket_proto proto = sio_servflow_sockproto(type);
    struct sio_server *serv = sio_server_create2(proto, threads);
    SIO_COND_CHECK_RETURN_VAL(!serv, NULL);

    union sio_server_opt ops = {
        .ops.accept = sio_server_newconn
    };
    sio_server_setopt(serv, SIO_SERV_OPS, &ops);

    return serv;
}

static inline
int sio_taskflow_create(struct sio_servflow *flow, unsigned short size)
{
    struct sio_servflow_taskflow *tflow = &flow->tflow;

    tflow->tf = sio_taskfifo_create(size, sio_sockflow_dataproc, flow, 1024);
    SIO_COND_CHECK_RETURN_VAL(tflow->tf == NULL, -1);

    return 0;
}

static inline
int sio_taskflow_destory(struct sio_servflow *flow)
{
    struct sio_servflow_taskflow *tflow = &flow->tflow;

    sio_taskfifo_destory(tflow->tf);

    return 0;
}

static inline
struct sio_servflow *sio_servflow_create_imp(enum sio_servflow_proto type, sio_thread_capacity io, sio_thread_capacity flow)
{
    struct sio_servflow *servflow = malloc(sizeof(struct sio_servflow));
    SIO_COND_CHECK_RETURN_VAL(!flow, NULL);

    memset(servflow, 0, sizeof(struct sio_servflow));

    struct sio_server *serv = sio_servflow_server_create(type, io);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!serv, NULL,
        free(servflow));

    union sio_server_opt ops = {
        .private = servflow
    };
    sio_server_setopt(serv, SIO_SERV_PRIVATE, &ops);

    servflow->serv = serv;

    int ret = sio_taskflow_create(servflow, flow);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, NULL,
        sio_server_destory(serv),
        free(servflow));

    return servflow;
}

static inline
void sio_servflow_set_ops(struct sio_servflow *flow, struct sio_servflow_ops *ops)
{
    struct sio_servflow_owner *owner = &flow->owner;
    owner->ops = *ops;
}

struct sio_servflow *sio_servflow_create(enum sio_servflow_proto type)
{
    return sio_servflow_create_imp(type, 1, 1);
}

struct sio_servflow *sio_servflow_create2(enum sio_servflow_proto type, sio_thread_capacity io, sio_thread_capacity flow)
{
    SIO_COND_CHECK_RETURN_VAL(io == 0 || flow == 0, NULL);

    return sio_servflow_create_imp(type, io, flow);
}

int sio_servflow_setopt(struct sio_servflow *flow, enum sio_servflow_optcmd cmd, union sio_servflow_opt *opt)
{
    SIO_COND_CHECK_RETURN_VAL(!flow || !opt, -1);

    int ret = 0;
    switch (cmd) {
    case SIO_SERVFLOW_OPS:
        sio_servflow_set_ops(flow, &opt->ops);
        break;

    default:
        ret = -1;
        break;
    }

    return ret;
}

int sio_servflow_listen(struct sio_servflow *flow, struct sio_servflow_addr *addr)
{
    SIO_COND_CHECK_RETURN_VAL(!flow || !addr, -1);

    struct sio_socket_addr skaddr = { 0 };
    memcpy(skaddr.addr, addr->addr, sizeof(addr->addr));
    skaddr.port = addr->port;

    return sio_server_listen(flow->serv, &skaddr);
}

static inline
void sio_sockflow_set_private(struct sio_sockflow *flow, void *private)
{
    flow->pri = private;
}

static inline
void sio_sockflow_get_private(struct sio_sockflow *flow, void **private)
{
    *private = flow->pri;
}

int sio_sockflow_setopt(struct sio_sockflow *flow, enum sio_sockflow_optcmd cmd, union sio_sockflow_opt *opt)
{
    int ret = 0;
    switch (cmd) {
    case SIO_SOCKFLOW_PRIVATE:
        sio_sockflow_set_private(flow, opt->private);
        break;

    default:
        ret = -1;
        break;
    }

    return ret;
}

int sio_sockflow_getopt(struct sio_sockflow *flow, enum sio_sockflow_optcmd cmd, union sio_sockflow_opt *opt)
{
    int ret = 0;
    switch (cmd) {
    case SIO_SOCKFLOW_PRIVATE:
        sio_sockflow_get_private(flow, &opt->private);
        break;

    default:
        ret = -1;
        break;
    }

    return ret;
}

int sio_sockflow_write(struct sio_sockflow *flow, char *buf, int len)
{
    struct sio_socket *sock = (struct sio_socket *)((char *)flow + sizeof(struct sio_sockflow));
    return sio_socket_write(sock, buf, len);
}

int sio_sockflow_close(struct sio_sockflow *flow)
{
    struct sio_socket *sock = (struct sio_socket *)((char *)flow + sizeof(struct sio_sockflow));
    return sio_socket_shutdown(sock, SIO_SOCK_SHUTRDWR);
}

int sio_servflow_destory(struct sio_servflow *flow)
{
    SIO_COND_CHECK_RETURN_VAL(!flow, -1);

    int ret = sio_server_destory(flow->serv);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    sio_taskflow_destory(flow);

    free(flow);

    return 0;
}