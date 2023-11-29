#include <stdio.h>
#include <string.h>
#include "sio_socket.h"
#include "sio_permplex.h"
#include "sio_log.h"

int socknew(struct sio_socket *serv);
int handshaked(struct sio_socket *serv);
int readable(struct sio_socket *sock);
int writeable(struct sio_socket *sock);
int closed(struct sio_socket *sock);

struct sio_sockops g_serv_ops = 
{
    .readable = socknew,
    .closeable = closed
};

struct sio_sockops g_sock_ops = 
{
    .handshaked = handshaked,
    .readable = readable,
    .writeable = writeable,
    .closeable = closed
};

struct sio_mplex *g_mplex = NULL;

int socknew(struct sio_socket *serv)
{
    int ret = sio_socket_accept_has_pend(serv);
    if (ret != 0) {
        return ret;
    }

    struct sio_socket *sock = sio_socket_create(SIO_SOCK_SSL, NULL);

    union sio_sockopt opt = { 0 };
    opt.ops = g_sock_ops;
    sio_socket_setopt(sock, SIO_SOCK_OPS, &opt);

    opt.data = "../cert/ca.crt";
    sio_socket_setopt(sock, SIO_SOCK_SSL_CACERT, &opt);
    opt.data = "../cert/server.crt";
    sio_socket_setopt(sock, SIO_SOCK_SSL_USERCERT, &opt);
    opt.data = "../cert/server.key";
    sio_socket_setopt(sock, SIO_SOCK_SSL_USERKEY, &opt);

    opt.mplex = g_mplex;
    sio_socket_setopt(sock, SIO_SOCK_MPLEX, &opt);

    sio_socket_accept(serv, sock);

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
    const char *response = "hello, client\n";
    int ret = sio_socket_write(sock, response, strlen(response));
    SIO_LOGE("write len: %d, real: %d\n", ret, strlen(response));
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
    struct sio_permplex *pmplex = sio_permplex_create(SIO_MPLEX_SELECT);
    struct sio_mplex *mplex = sio_permplex_mplex_ref(pmplex);
    g_mplex = mplex;

    struct sio_socket *serv = sio_socket_create2(SIO_SOCK_TCP, NULL);
    if (!serv) {
        SIO_LOGE("socket create failed\n");
        return -1;
    }

    union sio_sockopt opt = { 0 };
    opt.mplex = mplex;
    sio_socket_setopt(serv, SIO_SOCK_MPLEX, &opt);

    opt.nonblock = 1;
    sio_socket_setopt(serv, SIO_SOCK_NONBLOCK, &opt);

    opt.ops = g_serv_ops;
    sio_socket_setopt(serv, SIO_SOCK_OPS, &opt);

    struct sio_sockaddr addr = {
        .addr = "127.0.0.1",
        .port = 8000
    };
    int ret = sio_socket_listen(serv, &addr);
    if (ret != 0) {
        SIO_LOGI("sock listen ret: %d\n", ret);
        return 0;
    }

    sio_socket_mplex(serv, SIO_EV_OPT_ADD, SIO_EVENTS_IN);

    getc(stdin);
    sio_socket_shutdown(serv, SIO_SOCK_SHUTRDWR);

    getc(stdin);

    return 0;
}
