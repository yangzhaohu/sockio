#include <stdio.h>
#include <string.h>
#include "sio_socket.h"
#include "sio_mplex.h"
#include "sio_permplex.h"
#include "sio_log.h"

int socknew(struct sio_socket *serv);
int readable(struct sio_socket *sock);
int writeable(struct sio_socket *sock);
int closed(struct sio_socket *sock);

struct sio_mplex *g_mplex = NULL;

struct sio_socket *g_sock = NULL;

struct sio_sockops g_serv_ops = 
{
    .readable = socknew,
    .closeable = closed
};

struct sio_sockops g_sock_ops = 
{
    .readable = readable,
    .writeable = writeable,
    .closeable = closed
};

int socknew(struct sio_socket *serv)
{
    int ret = sio_socket_accept_has_pend(serv);
    if (ret != 0) {
        return ret;
    }

    struct sio_socket *sock = sio_socket_create(SIO_SOCK_TCP, NULL);
    sio_socket_accept(serv, sock);
    g_sock = sock;

    union sio_sockopt opt = { 0 };
    opt.ops = g_sock_ops;
    sio_socket_setopt(sock, SIO_SOCK_OPS, &opt);

    opt.nonblock = 1;
    ret = sio_socket_setopt(sock, SIO_SOCK_NONBLOCK, &opt);
    if (ret == -1) {
        SIO_LOGI("socket nonlock set failed\n");
    }

    opt.reuseaddr = 1;
    sio_socket_setopt(sock, SIO_SOCK_REUSEADDR, &opt);
    if (ret == -1) {
        SIO_LOGI("socket reuseaddr set failed\n");
    }

    opt.mplex = g_mplex;
    sio_socket_setopt(sock, SIO_SOCK_MPLEX, &opt);
    sio_socket_mplex(sock, SIO_EV_OPT_ADD, SIO_EVENTS_IN);

    return 0;
}

int readable(struct sio_socket *sock)
{
    char data[512] = { 0 };
    int len = sio_socket_read(sock, data, 512);
    if (len > 0 )
        SIO_LOGI("recv %d: %s\n", len, data);

    sio_socket_mplex(sock, SIO_EV_OPT_MOD, SIO_EVENTS_IN | SIO_EVENTS_OUT);

    return 0;
}

int closed(struct sio_socket *sock)
{
    SIO_LOGI("close\n");
    sio_socket_destory(sock);

    return 0;
}

char *g_resp = "HTTP/1.1 200 OK\r\n"
            "Connection: Keep-Alive\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 125\r\n\r\n"
            "<html>"
            "<head><title>Hello</title></head>"
            "<body>"
            "<center><h1>Hello, Client</h1></center>"
            "<hr><center>SOCKIO</center>"
            "</body>"
            "</html>";

int writeable(struct sio_socket *sock)
{
    SIO_LOGI("writeabled\n");
    sio_socket_mplex(sock, SIO_EV_OPT_MOD, SIO_EVENTS_IN);
    sio_socket_write(sock, g_resp, strlen(g_resp));
    return 0;
}

int main(void)
{
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

    struct sio_permplex *pmplex = sio_permplex_create(SIO_MPLEX_SELECT);
    struct sio_mplex *mplex = sio_permplex_mplex_ref(pmplex);
    g_mplex = mplex;

    opt.mplex = mplex;
    sio_socket_setopt(serv, SIO_SOCK_MPLEX, &opt);
    // SIO_LOGI("sample_socket_select set in\n");
    // getc(stdin);
    sio_socket_mplex(serv, SIO_EV_OPT_ADD, SIO_EVENTS_IN);

    getc(stdin);
    sio_socket_shutdown(serv, SIO_SOCK_SHUTRDWR);
    getc(stdin);

    getc(stdin);
    sio_permplex_destory(pmplex);
    
    return 0;
}