#include <stdio.h>
#include "sio_socket.h"
#include "sio_permplex.h"

int readable(struct sio_socket *sock)
{
    char data[512] = { 0 };
    int len = sio_socket_read(sock, data, 512);
    if (len > 0 )
        printf("recv %d: %s\n", len, data);

    sio_socket_mplex(sock, SIO_EV_OPT_MOD, SIO_EVENTS_IN | SIO_EVENTS_OUT);

    return 0;
}

int writeable(struct sio_socket *sock)
{
    printf("writeabled\n");
    sio_socket_mplex(sock, SIO_EV_OPT_MOD, SIO_EVENTS_IN);
    return 0;
}

int closed(struct sio_socket *sock)
{
    printf("close\n");
    sio_socket_destory(sock);

    return 0;
}

struct sio_socket_ops g_sock_ops = 
{
    .readable = readable,
    .writeable = writeable,
    .closeable = closed
};

int main()
{
    struct sio_permplex *pmplex = sio_permplex_create(SIO_MPLEX_SELECT);
    struct sio_mplex *mplex = sio_permplex_mplex_ref(pmplex);

    struct sio_socket *sock = sio_socket_create2(SIO_SOCK_TCP, NULL);

    union sio_socket_opt opt = { 0 };
    opt.ops = g_sock_ops;
    sio_socket_setopt(sock, SIO_SOCK_OPS, &opt);

    opt.mplex = mplex;
    sio_socket_setopt(sock, SIO_SOCK_MPLEX, &opt);

    opt.nonblock = 1;
    sio_socket_setopt(sock, SIO_SOCK_NONBLOCK, &opt);

    struct sio_socket_addr addr = {
        .addr = "110.242.68.66",
        // .addr = "69.171.242.11",
        .port = 80
    };
    printf("sock connect begin\n");
    int ret = sio_socket_connect(sock, &addr);
    printf("sock connect ret: %d\n", ret);

    getc(stdin);

    printf("socket shutdown\n");
    sio_socket_shutdown(sock, SIO_SOCK_SHUTRDWR);

    getc(stdin);

    printf("pmplex destory\n");
    sio_permplex_destory(pmplex);

    return 0;
}