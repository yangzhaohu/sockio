#include <stdio.h>
#include <string.h>
#include "sio_server.h"

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

static int socket_readable(void *ptr, const char *data, int len)
{
    struct sio_socket *sock = ptr;
    if (len == 0) {
        printf("client socket close\n");
        return 0;
    }

    sio_socket_mplex(sock, SIO_EV_OPT_MOD, SIO_EVENTS_IN | SIO_EVENTS_OUT);

    return 0;
}

static int socket_writeable(void *ptr, const char *data, int len)
{
    struct sio_socket *sock = ptr;

    sio_socket_mplex(sock, SIO_EV_OPT_MOD, SIO_EVENTS_IN);
    sio_socket_write(sock, g_resp, strlen(g_resp));
    return 0;
}

static int server_newconn(struct sio_socket *serv, struct sio_socket **sock)
{
    struct sio_socket *sock2 = sio_socket_create(SIO_SOCK_TCP);
    union sio_socket_opt opt = {
        .ops.read_cb = socket_readable,
        .ops.write_cb = socket_writeable
    };
    sio_socket_setopt(sock2, SIO_SOCK_OPS, &opt);
    opt.nonblock = 1;
    sio_socket_setopt(sock2, SIO_SOCK_NONBLOCK, &opt);

    int ret = sio_socket_accept(serv, &sock2);
    if(ret == -1) {
        sio_socket_destory(sock2);
        sock2 = NULL;
    }

    *sock = sock2;
    return ret;
}

static int server_close(struct sio_server *sock)
{
    printf("server close\n");
    return 0;
}

int main()
{
    struct sio_server *serv = sio_server_create(SIO_SOCK_TCP);

    union sio_server_opt ops = {
        .ops.accept_cb = server_newconn,
        .ops.close_cb = server_close
    };
    sio_server_setopt(serv, SIO_SERV_OPS, &ops);

    struct sio_socket_addr addr = {"127.0.0.1", 8000};
    sio_server_listen(serv, &addr);

#define SOCKET_CONNECT_TEST_COUNT 16
    struct sio_socket *client[SOCKET_CONNECT_TEST_COUNT] = { NULL };
    for (int i = 0; i < SOCKET_CONNECT_TEST_COUNT; i++) {
        client[i] = sio_socket_create(SIO_SOCK_TCP);
        sio_socket_connect(client[i], &addr);
        sio_socket_write(client[i], "1234", strlen("1234"));
    }

    getc(stdin);

    for (int i = 0; i < SOCKET_CONNECT_TEST_COUNT; i++) {
        sio_socket_shutdown(client[i], SIO_SOCK_SHUTRDWR);
    }

    getc(stdin);

    sio_server_destory(serv);

    getc(stdin);

    return 0;
}