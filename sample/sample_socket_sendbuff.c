#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "sio_socket.h"
#include "sio_errno.h"
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

int socknew(void *pri, const char *buf, int len)
{
    struct sio_socket *serv = pri;
    for (;;) {
        struct sio_socket *sock = sio_socket_create(SIO_SOCK_TCP);
        int ret = sio_socket_accept(serv, &sock);
        if (ret == SIO_ERRNO_AGAIN) {
            break;
        } else if (ret != 0) {
            printf("server close\n");
            sio_socket_destory(serv);
            return -1;
        }
        printf("new socket connect\n");

        union sio_socket_opt opt = { 0 };
        opt.ops = g_sock_ops;
        sio_socket_setopt(sock, SIO_SOCK_OPS, &opt);

        opt.nonblock = 1;
        ret = sio_socket_setopt(sock, SIO_SOCK_NONBLOCK, &opt);
        if (ret == -1) {
            printf("socket nonlock set failed\n");
        }

        opt.buff.rcvbuf = opt.buff.sndbuf = 4096;
        sio_socket_setopt(serv, SIO_SOCK_BUFF, &opt);

        g_sock = sock;

        sio_socket_mplex_bind(sock, g_mplex);
        sio_socket_mplex(sock, SIO_EV_OPT_ADD, SIO_EVENTS_IN);
    }

    return 0;
}

int readable(void *pri, const char *buf, int len)
{
    struct sio_socket *sock = pri;
    if (len == 0) {
        printf("socket close\n");
        sio_socket_destory(sock);
        return 0;
    }
    printf("recv %d: %s\n", len, buf);
    
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
    sio_socket_setopt(serv, SIO_SOCK_OPS, &opt);

    opt.nonblock = 1;
    sio_socket_setopt(serv, SIO_SOCK_NONBLOCK, &opt);

    struct sio_mplex_thread *mpthr = sio_mplex_thread_create(SIO_MPLEX_EPOLL);
    if (mpthr == NULL) {
        return -1;
    }

    g_mplex = sio_mplex_thread_mplex_ref(mpthr);

    sio_socket_mplex_bind(serv, g_mplex);
    sio_socket_mplex(serv, SIO_EV_OPT_ADD, SIO_EVENTS_IN);

    struct sio_socket *client = sio_socket_create(SIO_SOCK_TCP);
    opt.buff.rcvbuf = opt.buff.sndbuf = 4096;
    int ret = sio_socket_setopt(serv, SIO_SOCK_BUFF, &opt);
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