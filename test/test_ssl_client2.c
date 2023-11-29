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

int handshaked(struct sio_socket *sock)
{
    SIO_LOGI("handshaked succeed\n");
    sio_socket_mplex(sock, SIO_EV_OPT_MOD, SIO_EVENTS_IN | SIO_EVENTS_OUT);

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
    SIO_LOGI("writeabled\n");
    const char *request = "GET / HTTP/1.0\r\n"
                        "Connection: Keep-Alive\r\n"
                        "Host: 127.0.0.1:8000\r\n"
                        "User-Agent: ApacheBench/2.3\r\n"
                        "Accept: */*\r\n\r\n";
    int ret = sio_socket_write(sock, request, strlen(request));
    SIO_LOGE("write len: %d, real: %d\n", ret, strlen(request));
    sio_socket_mplex(sock, SIO_EV_OPT_MOD, SIO_EVENTS_IN);
    return 0;
}

int closed(struct sio_socket *sock)
{
    SIO_LOGI("close\n");
    // sio_socket_destory(sock);

    return 0;
}

int main()
{
    struct sio_permplex *pmplex = sio_permplex_create(SIO_MPLEX_EPOLL);
    struct sio_mplex *mplex = sio_permplex_mplex_ref(pmplex);

    struct sio_socket *sock = sio_socket_create2(SIO_SOCK_SSL, NULL);
    if (!sock) {
        SIO_LOGE("socket create failed\n");
        return -1;
    }

    union sio_sockopt opt = { 0 };
    opt.mplex = mplex;
    sio_socket_setopt(sock, SIO_SOCK_MPLEX, &opt);

    opt.nonblock = 1;
    sio_socket_setopt(sock, SIO_SOCK_NONBLOCK, &opt);

    struct sio_sockops ops =
    {
        .connected = connected,
        .handshaked = handshaked,
        .readable = readable,
        .writeable = writeable,
        .closeable = closed
    };
    opt.ops = ops;
    sio_socket_setopt(sock, SIO_SOCK_OPS, &opt);

    // opt.data = "../cert/ca.crt";
    // sio_socket_setopt(sock, SIO_SOCK_SSL_CACERT, &opt);
    // opt.data = "../cert/client.crt";
    // sio_socket_setopt(sock, SIO_SOCK_SSL_CACERT, &opt);
    // opt.data = "../cert/client.key";
    // sio_socket_setopt(sock, SIO_SOCK_SSL_CACERT, &opt);

    struct sio_sockaddr addr = {
        // .addr = "39.156.66.10",
        // .addr = "199.16.156.11", // connect timeout
        .addr = "127.0.0.1",
        .port = 8000
    };
    int ret = sio_socket_connect(sock, &addr);

    sio_socket_mplex(sock, SIO_EV_OPT_ADD, SIO_EVENTS_IN);

    getc(stdin);
    sio_socket_shutdown(sock, SIO_SOCK_SHUTRDWR);

    getc(stdin);

    return 0;
}
