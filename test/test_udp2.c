#include <stdio.h>
#include <string.h>
#include "sio_socket.h"
#include "sio_permplex.h"

static int socket_readable(struct sio_socket *sock)
{
    char buf[256] = { 0 };
    struct sio_socket_addr addr = { 0 };
    int ret = sio_socket_readfrom(sock, buf, 255, &addr);
    if (ret > 0) {
        printf("recv: %s\n    form: %s:%d\n\n", buf, addr.addr, addr.port);
    }

    return 0;
}

static int socket_writeable(struct sio_socket *sock)
{
    return 0;
}

static int socket_closeable(struct sio_socket *sock)
{
    printf("socket close\n");
    sio_socket_destory(sock);
    return 0;
}

int set_sock_mplex(struct sio_socket *sock, struct sio_permplex *mpt)
{
    union sio_socket_opt opt = { 0 };
    opt.ops.readfromable = socket_readable;
    opt.ops.closeable = socket_closeable;
    int ret = sio_socket_setopt(sock, SIO_SOCK_OPS, &opt);

    opt.nonblock = 1;
    ret = sio_socket_setopt(sock, SIO_SOCK_NONBLOCK, &opt);

    struct sio_mplex *mplex = sio_permplex_mplex_ref(mpt);
    opt.mplex = mplex;
    sio_socket_setopt(sock, SIO_SOCK_MPLEX, &opt);

    sio_socket_mplex(sock, SIO_EV_OPT_ADD, SIO_EVENTS_IN);

    return 0;
}

int main()
{
    struct sio_permplex *mpt = sio_permplex_create(SIO_MPLEX_EPOLL);

    struct sio_socket *sock = sio_socket_create(SIO_SOCK_UDP, NULL);
    struct sio_socket_addr addr = { "127.0.0.1", 8090};
    sio_socket_bind(sock, &addr);

    set_sock_mplex(sock, mpt);

    struct sio_socket *sock2 = sio_socket_create(SIO_SOCK_UDP, NULL);
    struct sio_socket_addr addr2 = { "127.0.0.1", 8091};
    sio_socket_bind(sock2, &addr2);

    set_sock_mplex(sock2, mpt);

    while (1) {
        char ch = getchar();
        if (ch == 'q') {
            sio_socket_shutdown(sock, SIO_SOCK_SHUTRDWR);
            sio_socket_shutdown(sock2, SIO_SOCK_SHUTRDWR);
            break;
        }
        if (ch == '2') {
            sio_socket_writeto(sock, "hello sock2", strlen("hello sock2"), &addr2);
            printf("send: %s\n    to: %s:%d\n", "hello sock2", addr2.addr, addr2.port);
        } else if (ch == '1'){
            sio_socket_writeto(sock2, "hello sock1", strlen("hello sock1"), &addr);
            printf("send: %s\n    to: %s:%d\n", "hello sock1", addr.addr, addr.port);
        }
    }

    sio_permplex_destory(mpt);

    return 0;
}