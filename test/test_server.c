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
    union sio_sockopt opt = { 0 };
    sio_socket_getopt(sock, SIO_SOCK_FD, &opt);
    SIO_LOGI("close fd: %d\n", opt.fd);
    sio_socket_destory(sock);

    return 0;
}

int socknew(struct sio_server *serv)
{
    struct sio_socket *sock = NULL;
    int ret = sio_server_accept(serv, &sock);
    SIO_COND_CHECK_RETURN_VAL(ret != 0, 0);

    union sio_sockopt opt = {
        .ops.readable = readable,
        .ops.writeable = writeable,
        .ops.closeable = closed
    };
    sio_socket_setopt(sock, SIO_SOCK_OPS, &opt);

    opt.nonblock = 1;
    sio_socket_setopt(sock, SIO_SOCK_NONBLOCK, &opt);

    return 0;
}

int readable_async(struct sio_socket *sock, const char *data, int len)
{
    // SIO_LOGI("recv %d: %s\n", len, data);

    int l = strlen(g_resp);
    char *buf = sio_aiobuf_alloc(l);
    memcpy(buf, g_resp, strlen(g_resp));
    sio_socket_async_write(sock, buf, l);

    sio_socket_async_read(sock, (char *)data, 512);

    return 0;
}

int writeable_async(struct sio_socket *sock, const char *data, int len)
{
    // SIO_LOGI("write %.*s\n", len, data);
    sio_aiobuf_free((sio_aiobuf)data);
    return 0;
}

int socknew_async(struct sio_server *serv)
{
    SIO_LOGI("new sock\n");
    struct sio_socket *sock = NULL;
    int ret = sio_server_accept(serv, &sock);
    SIO_COND_CHECK_RETURN_VAL(ret != 0, 0);

    union sio_sockopt opt = {
        .ops.readasync = readable_async,
        .ops.writeasync = writeable_async,
        .ops.closeable = closed
    };
    sio_socket_setopt(sock, SIO_SOCK_OPS, &opt);

    opt.nonblock = 1;
    sio_socket_setopt(sock, SIO_SOCK_NONBLOCK, &opt);

    char *buf = sio_aiobuf_alloc(512 * sizeof(char));
    memset(buf, 0, 512);
    sio_socket_async_read(sock, buf, 512);

    return 0;
}

static struct sio_servops g_servops[] = {
    [SIO_SERV_NIO] = {
        .newconnection = socknew
    },
    [SIO_SERV_AIO] = {
        .newconnection = socknew_async
    }
};

#define TEST_SERVER_IO SIO_SERV_NIO

int main(int argc, char *argv[])
{
    sio_logg_enable_prefix(0);
    sio_server_set_default(TEST_SERVER_IO);
    int cmd = 0;
    struct sio_server *serv = NULL;
    union sio_servopt opt;
    while ((cmd = getopt(argc, argv, "p:c:k:a:v:t:h")) != -1) {
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
            case 't':
                sio_logg_setlevel(atoi(optarg));
                break;

            case 'h':
                SIO_LOGI("Usage: test_server [destination] [port] [options]\n");
                SIO_LOGI("Options are:\n");
                SIO_LOGI("    -p proto       network proto type, 0: TCP, 2: SSL/TLS\n");
                SIO_LOGI("    -c usercert    user cert filename\n");
                SIO_LOGI("    -k userkey     user key filename\n");
                SIO_LOGI("    -a cacert      cacert filename\n");
                SIO_LOGI("    -v verify      verify peer, 0: disable, 1: enable\n");
                SIO_LOGI("    -t print       print level\n");
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

    opt.ops.newconnection = g_servops[TEST_SERVER_IO].newconnection;
    sio_server_setopt(serv, SIO_SERV_OPS, &opt);

    sio_server_listen(serv, &g_addr);

    getc(stdin);

    sio_server_destory(serv);

    getc(stdin);

    return 0;
}