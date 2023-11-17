#include <stdio.h>
#include <string.h>
#include "sio_socket.h"
#include "sio_permplex.h"
#include "sio_errno.h"

struct sio_mplex *g_mplex = NULL;

struct sio_socket *g_sock = NULL;

int socknew(struct sio_socket *serv, const char *buf, int len);
int readable(struct sio_socket *sock, const char *buf, int len);

struct sio_socket_ops g_serv_ops = 
{
    .read = socknew
};

struct sio_socket_ops g_sock_ops = 
{
    .read = readable
};

int socknew(struct sio_socket *serv, const char *buf, int len)
{
    SIO_LOGI("socket new connect comein\n");
    int ret = sio_socket_accept_has_pend(serv);
    if (ret == SIO_ERRNO_AGAIN) {
        return 0;
    } else if (ret != 0) {
        SIO_LOGI("server close\n");
        sio_socket_destory(serv);
        return -1;
    }


    struct sio_socket *sock = sio_socket_create(SIO_SOCK_TCP, NULL);
    sio_socket_accept(serv, sock);

    union sio_socket_opt opt = { 0 };
    opt.ops = g_sock_ops;
    sio_socket_setopt(sock, SIO_SOCK_OPS, &opt);

    opt.nonblock = 1;
    ret = sio_socket_setopt(sock, SIO_SOCK_NONBLOCK, &opt);
    if (ret == -1) {
        SIO_LOGI("socket nonlock set failed\n");
    }

    g_sock = sock;

    // opt.mplex = g_mplex;
    // sio_socket_setopt(sock, SIO_SOCK_MPLEX, &opt);
    // sio_socket_mplex(sock, SIO_EV_OPT_ADD, SIO_EVENTS_IN);

    return 0;
}

int readable(struct sio_socket *sock, const char *buf, int len)
{
    if (len == 0) {
        SIO_LOGI("socket close\n");
        return 0;
    }
    SIO_LOGI("recv %d: %s\n", len, buf);

    return 0;
}

int main()
{
    // server create
    struct sio_socket *serv = sio_socket_create(SIO_SOCK_TCP, NULL);

    struct sio_socket_addr addr = {"127.0.0.1", 8000};
    if (sio_socket_listen(serv, &addr) == -1) {
        SIO_LOGI("serv listen failed\n");
        return -1;
    }

    union sio_socket_opt opt = { 0 };
    opt.ops = g_serv_ops;
    sio_socket_setopt(serv, SIO_SOCK_OPS, &opt);

    opt.nonblock = 1;
    sio_socket_setopt(serv, SIO_SOCK_NONBLOCK, &opt);

    struct sio_mplex_thread *mpthr = sio_mplex_thread_create(SIO_MPLEX_EPOLL);
    if (mpthr == NULL) {
        return -1;
    }

    g_mplex = sio_mplex_thread_mplex_ref(mpthr);

    opt.mplex = g_mplex;
    sio_socket_setopt(serv, SIO_SOCK_MPLEX, &opt);

    sio_socket_mplex(serv, SIO_EV_OPT_ADD, SIO_EVENTS_IN);

    SIO_LOGI("please enter\n");

    // client socket connect
    getc(stdin);
    SIO_LOGI("step1: connect\n");

    struct sio_socket *client = sio_socket_create(SIO_SOCK_TCP, NULL);
    sio_socket_connect(client, &addr);

    opt.ops = g_sock_ops;
    sio_socket_setopt(client, SIO_SOCK_OPS, &opt);

    opt.nonblock = 1;
    sio_socket_setopt(client, SIO_SOCK_NONBLOCK, &opt);

    opt.mplex = g_mplex;
    sio_socket_setopt(client, SIO_SOCK_MPLEX, &opt);
    sio_socket_mplex(client, SIO_EV_OPT_ADD, SIO_EVENTS_IN);

    // send data
    getc(stdin);
    SIO_LOGI("step2: send test data\n");
    sio_socket_write(client, "hello server\n", strlen("hello server\n"));

    // destory client
    getc(stdin);
    SIO_LOGI("step3: destory client\n");
    sio_socket_destory(client);

    // delay mplex
    getc(stdin);
    SIO_LOGI("step4: server accept remote socket delay mplex\n");
    opt.mplex = g_mplex;
    sio_socket_setopt(g_sock, SIO_SOCK_MPLEX, &opt);
    sio_socket_mplex(g_sock, SIO_EV_OPT_ADD, SIO_EVENTS_IN);

    getc(stdin);

    return 0;
}