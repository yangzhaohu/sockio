#include <stdio.h>
#include <string.h>
#include "sio_socket.h"
#include "sio_mplex.h"
#include "sio_pmplex.h"

int socknew(struct sio_socket *serv, const char *buf, int len);
int readable(struct sio_socket *sock, const char *buf, int len);
int writeable(struct sio_socket *sock, const char *buf, int len);

struct sio_mplex *g_mplex = NULL;

struct sio_socket *g_sock = NULL;

struct sio_sockops g_serv_ops = 
{
    .read = socknew
};

struct sio_sockops g_sock_sock = 
{
    .read = readable,
    .write = writeable
};

int socknew(struct sio_socket *serv, const char *buf, int len)
{
    struct sio_socket *sock = sio_socket_create(SIO_SOCK_TCP, NULL);
    if (sio_socket_accept(serv, sock) == -1) {
        return -1;
    }
    g_sock = sock;

    union sio_sockopt opt = { 0 };
    opt.nonblock = 1;
    int ret = sio_sockopt(sock, SIO_SOCK_NONBLOCK, &opt);
    if (ret == -1)
        SIO_LOGI("socket nonlock set failed\n");
    opt.reuseaddr = 1;
    sio_sockopt(sock, SIO_SOCK_REUSEADDR, &opt);
    if (ret == -1)
        SIO_LOGI("socket reuseaddr set failed\n");

    // char buff[256];
    // sio_socket_read(sock, buff, 256);

    sio_socket_mplex_bind(sock, g_mplex);
    sio_socket_mplex(sock, 1, SIO_EVENTS_IN);

    return 0;
}

int readable(struct sio_socket *sock, const char *buf, int len)
{
    if (len == 0) {
        SIO_LOGI("close\n");
        return 0;
    }
    SIO_LOGI("recv %d: %s\n", len, buf);

    sio_socket_mplex(sock, SIO_EV_OPT_MOD, SIO_EVENTS_IN | SIO_EVENTS_OUT);

    return 0;
}

int writeable(struct sio_socket *sock, const char *buf, int len)
{
    SIO_LOGI("send: hello client\n");
    sio_socket_mplex(sock, SIO_EV_OPT_MOD, SIO_EVENTS_IN);
    sio_socket_write(sock, "hello client", strlen("hello client"));
    return 0;
}

#ifdef _WIN32
#include <winsock2.h>
#endif

int main(void)
{
#ifdef _WIN32
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        SIO_LOGI("WSAStartup failed with error: %d\n", err);
        return -1;
    }
#endif

    struct sio_socket_config config = {
        .proto = SIO_SOCK_TCP,
        .imm = 0,
        .ops = g_serv_ops,
        .owner = NULL
    };
    struct sio_socket *serv = sio_socket_create(&config);

    struct sio_sockaddr addr = {"127.0.0.1", 8000};
    if (sio_socket_listen(serv, &addr) == -1) {
        SIO_LOGI("serv listen failed\n");
        return -1;
    }


    union sio_sockopt opt = { 0 };
    opt.nonblock = 1;
    sio_sockopt(serv, SIO_SOCK_NONBLOCK, &opt);

    struct sio_mplex_attr attr = { SIO_MPLEX_SELECT };
    struct sio_mplex *mplex = sio_mplex_create(&attr);
    sio_socket_mplex_bind(serv, mplex);
    sio_socket_mplex(serv, 1, SIO_EVENTS_IN);

    /*struct sio_pmplex *smt = */sio_pmplex_create2(mplex);

    // socket rw mplex
    // struct sio_pmplex *smt_io = sio_pmplex_create(0);
    g_mplex = mplex; // sio_pmplex_mplex_ref(smt_io);

    getc(stdin);
    sio_socket_close(g_sock);
    getc(stdin);

#ifdef _WIN32
    WSACleanup();
#endif
    
    return 0;
}