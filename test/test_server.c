#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include "sio_common.h"
#include "sio_server.h"
#include "sio_log.h"

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

static enum sio_sockprot g_prot = SIO_SOCK_TCP;
static struct sio_sockaddr g_addr = {
    .addr = "127.0.0.1",
    .port = 8000
};

int readable(struct sio_socket *sock)
{
    char data[512] = { 0 };
    sio_socket_read(sock, data, 512);

    sio_socket_mplex(sock, SIO_EV_OPT_MOD, SIO_EVENTS_OUT);

    return 0;
}

int writeable(struct sio_socket *sock)
{
    int ret = sio_socket_write(sock, g_resp, strlen(g_resp));
    sio_socket_mplex(sock, SIO_EV_OPT_MOD, SIO_EVENTS_IN);

    return 0;
}

int closed(struct sio_socket *sock)
{
    SIO_LOGI("close\n");
    sio_socket_destory(sock);

    return 0;
}

int socknew(struct sio_server *serv)
{
    struct sio_socket *sock = sio_socket_create(g_prot, NULL);
    union sio_sockopt opt = {
        .ops.readable = readable,
        .ops.writeable = writeable,
        .ops.closeable = closed
    };
    sio_socket_setopt(sock, SIO_SOCK_OPS, &opt);

    opt.nonblock = 1;
    sio_socket_setopt(sock, SIO_SOCK_NONBLOCK, &opt);

    int ret = sio_server_accept(serv, sock);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        sio_socket_destory(sock));

    ret = sio_server_socket_mplex(serv, sock);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        sio_socket_destory(sock));

    return 0;
}

int main(int argc, char *argv[])
{
    int cmd = 0;
    struct sio_server *serv = NULL;
    union sio_servopt opt;
    while ((cmd = getopt(argc, argv, "p:c:k:a:v:h")) != -1) {
        switch (cmd) {
            case 'p':
                if (atoi(optarg) == 0) {
                    g_prot = SIO_SOCK_TCP;
                    serv = sio_server_create(SIO_SOCK_TCP);
                } else if (atoi(optarg) == 2) {
                    g_prot = SIO_SOCK_SSL;
                    serv = sio_server_create(SIO_SOCK_SSL);
                }
                break;

            case 'c':
                opt.data = optarg;
                sio_server_setopt(serv, SIO_SERV_SSL_USERCERT, &opt);
                break;

            case 'k':
                opt.data = optarg;
                sio_server_setopt(serv, SIO_SERV_SSL_USERKEY, &opt);
                break;

            case 'a':
                opt.data = optarg;
                sio_server_setopt(serv, SIO_SERV_SSL_CACERT, &opt);
                break;

            case 'v':
                if (atoi(optarg) == 1) {
                    opt.enable = 1;
                    sio_server_setopt(serv, SIO_SERV_SSL_VERIFY_PEER, &opt);
                }
                break;

            case 'h':
                SIO_LOGI("Usage: test_server [destination] [port] [options]\n");
                SIO_LOGI("Options are:\n");
                SIO_LOGI("    -p proto       network proto type, 0: TCP, 2: SSL/TLS\n");
                SIO_LOGI("    -c usercert    user cert filename\n");
                SIO_LOGI("    -k userkey     user key filename\n");
                SIO_LOGI("    -k cacert      cacert filename\n");
                SIO_LOGI("    -v verify      verify peer, 0: disable, 1: enable\n");
                return 0;
        }
    }

    // try get an host name
    if (optind < argc) {
        SIO_LOGI("hostname: %s\n", argv[optind]);
        optind++;
    }

    // loop all other parameters for port-ranges
    while (optind < argc) {
        optind++;
        /* code */
    }

    SIO_LOGI("server:\n"
            "    addr: %s\n"
            "    port: %d\n"
            "    proto: %s\n", 
            g_addr.addr, g_addr.port, g_prot == 0 ? "TCP" : "SSL");
    
    if (serv == NULL) {
        serv = sio_server_create(g_prot);
    }

    opt.ops.accept = socknew;
    sio_server_setopt(serv, SIO_SERV_OPS, &opt);

    sio_server_listen(serv, &g_addr);

    getc(stdin);

    sio_server_destory(serv);

    getc(stdin);

    return 0;
}