#include <stdio.h>
#include <string.h>
#include "sio_socket.h"
#include "sio_mplex_thread.h"

struct sio_mplex *g_mplex = NULL;

struct sio_socket *g_sock = NULL;

int socknew(void *ptr, const char *buf, int len);
int readable(void *ptr, const char *buf, int len);

struct sio_socket_ops g_serv_ops = 
{
    .read_cb = socknew
};

struct sio_socket_ops g_sock_ops = 
{
    .read_cb = readable
};

int socknew(void *ptr, const char *buf, int len)
{
    struct sio_socket *serv = ptr;
    struct sio_socket *sock = sio_socket_create(SIO_SOCK_TCP);
    if (sio_socket_accept(serv, sock) == -1) {
        printf("server close\n");
        return -1;
    }
    
    printf("new socket connect\n");

    union sio_socket_opt opt = { 0 };
    opt.ops = g_sock_ops;
    sio_socket_option(sock, SIO_SOCK_OPS, &opt);

    opt.nonblock = 1;
    int ret = sio_socket_option(sock, SIO_SOCK_NONBLOCK, &opt);
    if (ret == -1) {
        printf("socket nonlock set failed\n");
    }

    g_sock = sock;

    sio_socket_mplex_bind(sock, g_mplex);
    sio_socket_mplex(sock, SIO_EV_OPT_ADD, SIO_EVENTS_IN);

    return 0;
}

int readable(void *pri, const char *buf, int len)
{
    if (len == 0) {
        printf("socket close\n");
        return 0;
    }
    printf("recv %d: %s\n", len, buf);

    return 0;
}

int main()
{
    // server create
    struct sio_socket *serv = sio_socket_create(SIO_SOCK_TCP);

    struct sio_socket_addr addr = {"127.0.0.1", 8000};
    if (sio_socket_listen(serv, &addr) == -1) {
        printf("serv listen failed\n");
        return -1;
    }

    union sio_socket_opt opt = { 0 };
    opt.ops = g_serv_ops;
    sio_socket_option(serv, SIO_SOCK_OPS, &opt);

    opt.nonblock = 1;
    sio_socket_option(serv, SIO_SOCK_NONBLOCK, &opt);

    struct sio_mplex_thread *mpthr = sio_mplex_thread_create(SIO_MPLEX_EPOLL);
    if (mpthr == NULL) {
        return -1;
    }

    g_mplex = sio_mplex_thread_mplex_ref(mpthr);

    sio_socket_mplex_bind(serv, g_mplex);
    sio_socket_mplex(serv, SIO_EV_OPT_ADD, SIO_EVENTS_IN);

    struct sio_socket *client = sio_socket_create(SIO_SOCK_TCP);
    sio_socket_connect(client, &addr);

    getc(stdin);
    sio_socket_destory(client);

    getc(stdin);
    printf("g_sock destory\n");
    sio_socket_destory(g_sock);

    getc(stdin);
    printf("server destory\n");
    sio_socket_destory(serv);

    getc(stdin);
    printf("exit\n");

    return 0;
}