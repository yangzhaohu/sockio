#include <stdio.h>
#include <string.h>
#include "sio_socket.h"
#include "sio_permplex.h"
#include "sio_log.h"

int connected(struct sio_socket *sock, enum sio_sockwhat what)
{
    if (what == SIO_SOCK_ESTABLISHED) {
        SIO_LOGI("connect succeed\n");
    } else {
        SIO_LOGI("connect timeout\n");
    }

    return 0;
}

int readable(struct sio_socket *sock)
{
    char data[512] = { 0 };
    int len = sio_socket_read(sock, data, 512);
    if (len > 0 )
        SIO_LOGI("recv %d: %s\n", len, data);

    return 0;
}

int writeable(struct sio_socket *sock)
{
    char timebuf[256] = { 0 };
    SIO_LOGI("writeabled\n");
    int ret = sio_socket_write(sock, "1234", strlen("1234"));

    SIO_LOGI("write ret: %d\n", ret);
    return 0;
}

int closed(struct sio_socket *sock)
{
    SIO_LOGI("close\n");
    // sio_socket_destory(sock);

    return 0;
}

struct sio_sockops g_sock_ops = 
{
    .connected = connected,
    .readable = readable,
    .writeable = writeable,
    .closeable = closed
};

int main()
{
    struct sio_permplex *pmplex = sio_permplex_create(SIO_MPLEX_SELECT);
    struct sio_mplex *mplex = sio_permplex_mplex_ref(pmplex);

    struct sio_socket *sock = sio_socket_create2(SIO_SOCK_TCP, NULL);

    union sio_sockopt opt = { 0 };
    opt.ops = g_sock_ops;
    sio_socket_setopt(sock, SIO_SOCK_OPS, &opt);

    opt.mplex = mplex;
    sio_socket_setopt(sock, SIO_SOCK_MPLEX, &opt);

    opt.nonblock = 1;
    sio_socket_setopt(sock, SIO_SOCK_NONBLOCK, &opt);

    struct sio_sockaddr addr = {
        .addr = "110.242.68.66",
        // .addr = "199.16.156.11",
        .port = 80
    };
    char timebuf[256] = { 0 };
    SIO_LOGI("sock connect begin\n");
    int ret = sio_socket_connect(sock, &addr);
    SIO_LOGI("sock connect ret: %d\n", ret);

    getc(stdin);

    SIO_LOGI("socket shutdown\n");
    sio_socket_shutdown(sock, SIO_SOCK_SHUTRDWR);

    getc(stdin);

    SIO_LOGI("pmplex destory\n");
    sio_permplex_destory(pmplex);
    sio_socket_destory(sock);

    return 0;
}