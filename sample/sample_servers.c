#include <stdio.h>
#include <string.h>
#include "sio_servers.h"

static char *g_resp =
            "HTTP/1.1 200 OK\r\n"
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

int servers_new_conn(struct sio_socket *sock)
{
    printf("new conn enter\n");
    sio_socket_write(sock, g_resp, strlen(g_resp));
    return 0;
}

int main()
{
    struct sio_servers *servs = sio_servers_create2(SIO_SOCK_TCP, 16);

    union sio_servers_opt opt = {
        .ops.new_conn = servers_new_conn
    };
    sio_servers_setopt(servs, SIO_SERVS_OPS, &opt);

    struct sio_socket_addr addr = {"127.0.0.1", 8000};
    sio_servers_listen(servs, &addr);

    getc(stdin);

    sio_servers_destory(servs);

    getc(stdin);

    return 0;
}