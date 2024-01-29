#include <stdio.h>
#include <string.h>
#include "sio_common.h"
#include "sio_server.h"
#include "sio_log.h"

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

static int socket_readable(struct sio_socket *sock)
{
    sio_socket_mplex(sock, SIO_EV_OPT_MOD, SIO_EVENTS_IN | SIO_EVENTS_OUT);

    return 0;
}

static int socket_writeable(struct sio_socket *sock)
{
    sio_socket_mplex(sock, SIO_EV_OPT_MOD, SIO_EVENTS_IN);
    sio_socket_write(sock, g_resp, strlen(g_resp));
    return 0;
}

static int socket_close(struct sio_socket *sock)
{
    SIO_LOGI("client socket close\n");
    sio_socket_destory(sock);

    return 0;
}

static int server_newconn(struct sio_server *serv)
{
    struct sio_socket *sock = NULL;
    int ret = sio_server_accept(serv, &sock);
    SIO_COND_CHECK_RETURN_VAL(ret != 0, 0);

    union sio_sockopt opt = {
        .ops.readable = socket_readable,
        .ops.writeable = socket_writeable,
        .ops.closeable = socket_close
    };
    sio_socket_setopt(sock, SIO_SOCK_OPS, &opt);

    opt.nonblock = 1;
    sio_socket_setopt(sock, SIO_SOCK_NONBLOCK, &opt);

    return ret;
}

int main()
{
    struct sio_server *serv = sio_server_create(SIO_SOCK_TCP);

    union sio_servopt ops = {
        .ops.newconnection = server_newconn
    };
    sio_server_setopt(serv, SIO_SERV_OPS, &ops);

    struct sio_sockaddr addr = {"127.0.0.1", 8000};
    sio_server_listen(serv, &addr);

#define SOCKET_CONNECT_TEST_COUNT 16
    struct sio_socket *client[SOCKET_CONNECT_TEST_COUNT] = { NULL };
    for (int i = 0; i < SOCKET_CONNECT_TEST_COUNT; i++) {
        client[i] = sio_socket_create(SIO_SOCK_TCP, NULL);
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