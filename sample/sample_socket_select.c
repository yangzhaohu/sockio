#include <stdio.h>
#include <string.h>
#include "sio_socket.h"
#include "sio_mplex.h"
#include "sio_mplex_thread.h"

int socknew(void *ptr, const char *buf, int len);
int readable(void *ptr, const char *buf, int len);
int writeable(void *ptr, const char *buf, int len);

struct sio_mplex *g_mplex = NULL;

struct sio_socket *g_sock = NULL;

struct sio_socket_ops g_serv_ops = 
{
    .read_cb = socknew
};

struct sio_socket_ops g_sock_ops = 
{
    .read_cb = readable,
    .write_cb = writeable
};

int socknew(void *ptr, const char *buf, int len)
{
    struct sio_socket *serv = ptr;
    struct sio_socket *sock = sio_socket_create(SIO_SOCK_TCP);
    if (sio_socket_accept(serv, sock) == -1) {
        printf("server close\n");
        return -1;
    }
    g_sock = sock;

    union sio_socket_opt opt = { 0 };
    opt.ops = g_sock_ops;
    sio_socket_option(sock, SIO_SOCK_OPS, &opt);

    opt.nonblock = 1;
    int ret = sio_socket_option(sock, SIO_SOCK_NONBLOCK, &opt);
    if (ret == -1) {
        printf("socket nonlock set failed\n");
    }

    opt.reuseaddr = 1;
    sio_socket_option(sock, SIO_SOCK_REUSEADDR, &opt);
    if (ret == -1) {
        printf("socket reuseaddr set failed\n");
    }

    sio_socket_mplex_bind(sock, g_mplex);
    sio_socket_mplex(sock, SIO_EV_OPT_ADD, SIO_EVENTS_IN);

    return 0;
}

int readable(void *pri, const char *buf, int len)
{
    if (len == 0) {
        printf("close\n");
        return 0;
    }
    printf("recv %d: %s\n", len, buf);

    struct sio_socket *sock = pri;
    sio_socket_mplex(sock, SIO_EV_OPT_MOD, SIO_EVENTS_IN | SIO_EVENTS_OUT);

    return 0;
}

char *g_resp = "HTTP/1.1 200 OK\r\n"
            "Connection: close\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 125\r\n"
            "Connection: close\r\n\r\n"
            "<html>"
            "<head><title>Hello</title></head>"
            "<body>"
            "<center><h1>Hello, Client</h1></center>"
            "<hr><center>SOCKIO</center>"
            "</body>"
            "</html>";

int writeable(void *pri, const char *buf, int len)
{
    struct sio_socket *sock = pri;
    sio_socket_mplex(sock, SIO_EV_OPT_MOD, SIO_EVENTS_IN);
    sio_socket_write(sock, g_resp, strlen(g_resp));
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
        printf("WSAStartup failed with error: %d\n", err);
        return -1;
    }
#endif

    struct sio_socket *serv = sio_socket_create(SIO_SOCK_TCP);

    struct sio_socket_addr addr = {"127.0.0.1", 8000};
    if (sio_socket_listen(serv, &addr) == -1) {
        printf("serv listen failed\n");
        return -1;
    }


    union sio_socket_opt opt = { 0 };
    opt.ops = g_serv_ops;
    sio_socket_option(serv, SIO_SOCK_OPS, &opt);

    opt.nonblock = 1;
    sio_socket_option(serv, SIO_SOCK_NONBLOCK, &opt);

    struct sio_mplex_attr attr = { SIO_MPLEX_SELECT };
    struct sio_mplex *mplex = sio_mplex_create(&attr);
    g_mplex = mplex;

    sio_socket_mplex_bind(serv, mplex);
    sio_socket_mplex(serv, SIO_EV_OPT_ADD, SIO_EVENTS_IN);

    sio_mplex_thread_create2(mplex);
    
    getc(stdin);
    sio_socket_destory(serv);
    getc(stdin);

#ifdef _WIN32
    WSACleanup();
#endif
    
    return 0;
}