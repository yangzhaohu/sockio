#include <stdio.h>
#include <string.h>
#include "sio_socket.h"
#include "sio_errno.h"
#include "sio_pmplex.h"
#include "sio_log.h"

struct sio_mplex *g_mplex = NULL;

struct sio_socket *g_sock = NULL;

int socknew(struct sio_socket *serv);
int readable(struct sio_socket *sock);
int closed(struct sio_socket *sock);

struct sio_sockops g_serv_ops = 
{
    .readable = socknew
};

struct sio_sockops g_sock_ops = 
{
    .readable = readable,
    .closeable = closed
};

int socknew(struct sio_socket *serv)
{
    for (;;) {
        int ret = sio_socket_accept_has_pend(serv);
        if (ret == SIO_ERRNO_AGAIN) {
            break;
        } else if (ret != 0) {
            SIO_LOGI("server close\n");
            sio_socket_destory(serv);
            return -1;
        }
        SIO_LOGI("new socket connect\n");

        struct sio_socket *sock = sio_socket_create(SIO_SOCK_TCP, NULL);
        sio_socket_accept(serv, sock);

        union sio_sockopt opt = { 0 };
        opt.ops = g_sock_ops;
        sio_socket_setopt(sock, SIO_SOCK_OPS, &opt);

        opt.nonblock = 1;
        ret = sio_socket_setopt(sock, SIO_SOCK_NONBLOCK, &opt);
        if (ret == -1) {
            SIO_LOGI("socket nonlock set failed\n");
        }

        g_sock = sock;

        opt.mplex = g_mplex;
        sio_socket_setopt(sock, SIO_SOCK_MPLEX, &opt);

        sio_socket_mplex(sock, SIO_EV_OPT_ADD, SIO_EVENTS_IN);
    }

    return 0;
}

int readable(struct sio_socket *sock)
{
    char data[512] = { 0 };
    int len = sio_socket_read(sock, data, 512);
    SIO_LOGI("recv %d: %s\n", len, data);

    return 0;
}

int closed(struct sio_socket *sock)
{
    SIO_LOGI("close\n");
    sio_socket_destory(sock);

    return 0;
}

int main()
{
    // server create
    struct sio_socket *serv = sio_socket_create(SIO_SOCK_TCP, NULL);

    struct sio_sockaddr addr = {"127.0.0.1", 8000};
    if (sio_socket_listen(serv, &addr) == -1) {
        SIO_LOGI("serv listen failed\n");
        return -1;
    }

    union sio_sockopt opt = { 0 };
    opt.ops = g_serv_ops;
    sio_socket_setopt(serv, SIO_SOCK_OPS, &opt);

    opt.nonblock = 1;
    sio_socket_setopt(serv, SIO_SOCK_NONBLOCK, &opt);

    struct sio_pmplex *mpthr = sio_pmplex_create(SIO_MPLEX_EPOLL);
    if (mpthr == NULL) {
        return -1;
    }

    g_mplex = sio_pmplex_mplex_ref(mpthr);

    opt.mplex = g_mplex;
    sio_socket_setopt(serv, SIO_SOCK_MPLEX, &opt);

    sio_socket_mplex(serv, SIO_EV_OPT_ADD, SIO_EVENTS_IN);

    struct sio_socket *client = sio_socket_create(SIO_SOCK_TCP, NULL);
    sio_socket_connect(client, &addr);

    struct sio_socket *client2 = sio_socket_create(SIO_SOCK_TCP, NULL);
    sio_socket_connect(client2, &addr);

    getc(stdin);
    SIO_LOGI("the client is not mplex and can be safely destroyed\n");
    sio_socket_destory(client);

    getc(stdin);
    SIO_LOGI("The g_sock is mplexing. use shutdown in the mplex callback secure destruction\n");
    sio_socket_shutdown(g_sock, SIO_SOCK_SHUTRDWR);

    getc(stdin);
    SIO_LOGI("The serv is mplexing. use shutdown in the mplex callback secure destruction\n");
    sio_socket_shutdown(serv, SIO_SOCK_SHUTRDWR);

    getc(stdin);
    SIO_LOGI("exit\n");

    return 0;
}