#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "sio_socket.h"
#include "sio_errno.h"
#include "sio_mplex_thread.h"

struct sio_mplex *g_mplex = NULL;

struct sio_socket *g_sock = NULL;

int socknew(struct sio_socket *serv);
int readable(struct sio_socket *socknew);
int closed(struct sio_socket *sock);

struct sio_socket_ops g_serv_ops = 
{
    .readable = socknew
};

struct sio_socket_ops g_sock_ops = 
{
    .readable = readable
};

int socknew(struct sio_socket *serv)
{
    for (;;) {
        int ret = sio_socket_accept_has_pend(serv);
        if (ret == SIO_ERRNO_AGAIN) {
            break;
        } else if (ret != 0) {
            printf("server close\n");
            sio_socket_destory(serv);
            return -1;
        }

        struct sio_socket *sock = sio_socket_create(SIO_SOCK_TCP, NULL);
        sio_socket_accept(serv, sock);
        printf("new socket connect\n");

        union sio_socket_opt opt = { 0 };
        opt.ops = g_sock_ops;
        sio_socket_setopt(sock, SIO_SOCK_OPS, &opt);

        opt.nonblock = 1;
        ret = sio_socket_setopt(sock, SIO_SOCK_NONBLOCK, &opt);
        if (ret == -1) {
            printf("socket nonlock set failed\n");
        }

        opt.rcvbuf = opt.sndbuf = 4096;
        sio_socket_setopt(serv, SIO_SOCK_RCVBUF, &opt);

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
    if (len > 0)
        printf("recv %d: %s\n", len, data);
    
    char buff[10240] = { 0 };
    int cnt = 0;
    for (;;) {
        int sent = sio_socket_write(sock, buff, 1024);
        if (sent <= 0) {
            printf("buffer full: %d\n", cnt);
            break;
        }
        usleep(10000);
        cnt += sent;
        printf("send: %8d\n", cnt);
    }

    return 0;
}

int closed(struct sio_socket *sock)
{
    printf("close\n");
    sio_socket_destory(sock);

    return 0;
}

int main()
{
    // server create
    struct sio_socket *serv = sio_socket_create(SIO_SOCK_TCP, NULL);

    struct sio_socket_addr addr = {"127.0.0.1", 8000};
    if (sio_socket_listen(serv, &addr) == -1) {
        printf("serv listen failed\n");
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

    struct sio_socket *client = sio_socket_create(SIO_SOCK_TCP, NULL);
    opt.rcvbuf = opt.sndbuf = 4096;
    int ret = sio_socket_setopt(serv, SIO_SOCK_RCVBUF, &opt);
    if (ret == -1) {
        printf("buff size set failed\n");
    }

    sio_socket_connect(client, &addr);

    getc(stdin);
    sio_socket_write(client, "hello\n", strlen("hello\n"));

    getc(stdin);
    printf("exit\n");

    return 0;
}