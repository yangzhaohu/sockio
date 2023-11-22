#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sio_socket.h"
#include "sio_permplex.h"
#include "sio_log.h"

int socknew(struct sio_socket *serv, struct sio_socket *newsock);
int readable(struct sio_socket *sock, const char *data, int len);
int writeable(struct sio_socket *sock, const char *data, int len);
int closed(struct sio_socket *sock);

struct sio_sockops g_serv_ops = 
{
    .acceptasync = socknew
};

struct sio_sockops g_sock_ops = 
{
    .readasync = readable,
    .writeasync = writeable,
    .closeable = closed
};

struct sio_mplex *g_mplex = NULL;

int post_socket_accept(struct sio_socket *serv)
{
    struct sio_socket *sock = sio_socket_create2(SIO_SOCK_TCP, NULL);

    union sio_sockopt opt = { 0 };
    opt.nonblock = 1;
    sio_socket_setopt(sock, SIO_SOCK_NONBLOCK, &opt);
    opt.ops = g_sock_ops;
    sio_socket_setopt(sock, SIO_SOCK_OPS, &opt);
    opt.mplex = g_mplex;
    sio_socket_setopt(sock, SIO_SOCK_MPLEX, &opt);

    sio_socket_async_accept(serv, sock);

    return 0;
}

int socknew(struct sio_socket *serv, struct sio_socket *newsock)
{
    SIO_LOGI("new socket accpet\n");
    sio_socket_mplex(newsock, SIO_EV_OPT_ADD, SIO_EVENTS_IN);

    char *buff = malloc(512 * sizeof(char));
    memset(buff, 0, 512);
    sio_socket_async_read(newsock, buff, 512);

    post_socket_accept(serv);

    return 0;
}

int readable(struct sio_socket *sock, const char *data, int len)
{
    SIO_LOGI("recv %d: %s\n", len, data);
    sio_socket_async_read(sock, (char *)data, 512);

    return 0;
}

int writeable(struct sio_socket *sock, const char *data, int len)
{
    return 0;
}

int closed(struct sio_socket *sock)
{
    SIO_LOGI("close\n");
    sio_socket_destory(sock);
}

int main(void)
{
    // create mplex
    struct sio_permplex *pmplex = sio_permplex_create(SIO_MPLEX_IOCP);
    struct sio_mplex *mplex = sio_permplex_mplex_ref(pmplex);
    g_mplex = mplex;

    // create server socket
    struct sio_socket *serv = sio_socket_create(SIO_SOCK_TCP, NULL);

    // set nonblock
    union sio_sockopt opt = { 0 };
    opt.nonblock = 1;
    sio_socket_setopt(serv, SIO_SOCK_NONBLOCK, &opt);

    opt.ops = g_serv_ops;
    sio_socket_setopt(serv, SIO_SOCK_OPS, &opt);

    // listen 
    struct sio_sockaddr addr = {"127.0.0.1", 8000};
    if (sio_socket_listen(serv, &addr) == -1) {
        SIO_LOGI("serv listen failed\n");
        return -1;
    }

    // serv mplex
    opt.mplex = g_mplex;
    sio_socket_setopt(serv, SIO_SOCK_MPLEX, &opt);

    sio_socket_mplex(serv, SIO_EV_OPT_ADD, SIO_EVENTS_IN);

    // post socket accept
    post_socket_accept(serv);

    getc(stdin);
    
    return 0;
}
