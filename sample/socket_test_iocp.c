#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sio_socket.h"
#include "sio_mplex.h"
#include "sio_mplex_thread.h"

#ifdef _WIN32
#include <winsock2.h>
void win_sock_init(void)
{
    WORD version;
    WSADATA data;

    version = MAKEWORD(2, 2);
    int err = WSAStartup(version, &data);
    if (err != 0) {
        printf("WSAStartup failed with error: %d\n", err);
    }
}

void win_sock_cleanup()
{
    WSACleanup();
}
#else
void win_sock_init(void) {}
void win_sock_cleanup(void) {}
#endif

int socknew(struct sio_socket *serv, const char *buf, int len);
int readable(struct sio_socket *sock, const char *buf, int len);
int writeable(struct sio_socket *sock, const char *buf, int len);

struct sio_socket_ops g_serv_ops = 
{
    .read = socknew
};

struct sio_socket_ops g_sock_ops = 
{
    .read = readable,
    .write = writeable
};

struct sio_mplex *g_mplex2 = NULL;
struct sio_socket *g_sock = NULL;

int post_socket_accept(struct sio_socket *serv, struct sio_mplex *mplex)
{
    struct sio_socket_config config = {
        .proto = SIO_SOCK_TCP,
        .imm = 1,
        .ops = g_sock_ops,
        .owner = NULL
    };
    struct sio_socket *sock = sio_socket_create(&config);
    sio_socket_mplex_bind(sock, mplex);
    sio_socket_mplex(sock, SIO_EV_OPT_ADD, SIO_EVENTS_IN);
    sio_socket_async_accept(serv, sock);

    return 0;
}

int post_socket_read(struct sio_socket *sock)
{
    char *buff = malloc(512 * sizeof(char));
    memset(buff, 0, 512);
    sio_socket_async_read(sock, buff, 512);

    return 0;
}

int socknew(struct sio_socket *serv, const char *data, int len)
{
    struct sio_socket *sock = (struct sio_socket *)data;
    post_socket_read(sock);

    post_socket_accept(serv, g_mplex2);
    return 0;
}

int readable(struct sio_socket *sock, const char *data, int len)
{
    if (len <= 0) {
        printf("close\n");
        return 0;
    }

    g_sock = sock;
    printf("recv %d: %s\n", len, data);
    sio_socket_async_read(sock, (char *)data, 512);
    return 0;
}

int writeable(struct sio_socket *sock, const char *data, int len)
{
    return 0;
}

int main(void)
{
    win_sock_init();

    // create mplex
    struct sio_mplex_attr attr = { SIO_MPLEX_IOCP };
    struct sio_mplex *mplex = sio_mplex_create(&attr);
    struct sio_mplex *mplex2 = sio_mplex_create(&attr);
    g_mplex2 = mplex2;

    // create server socket
    struct sio_socket_config config = {
        .proto = SIO_SOCK_TCP,
        .imm = 0,
        .ops = g_serv_ops,
        .owner = NULL
    };
    struct sio_socket *serv = sio_socket_create(&config);

    // listen 
    struct sio_socket_addr addr = {"127.0.0.1", 8000};
    if (sio_socket_listen(serv, &addr) == -1) {
        printf("serv listen failed\n");
        return -1;
    }

    // set nonblock
    union sio_socket_opt opt = { 0 };
    opt.nonblock = 1;
    sio_socket_setopt(serv, SIO_SOCK_NONBLOCK, &opt);

    // serv mplex
    sio_socket_mplex_bind(serv, mplex);
    sio_socket_mplex(serv, SIO_EV_OPT_ADD, SIO_EVENTS_IN);

    // post socket accept
    post_socket_accept(serv, mplex2);

    // start thread process mplex
    sio_mplex_thread_create2(mplex);
    sio_mplex_thread_create2(mplex2);

    getc(stdin);
    sio_socket_close(g_sock);

    getc(stdin);
    win_sock_cleanup();
    
    return 0;
}
