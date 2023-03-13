#include "sio_servers.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sio_common.h"
#include "sio_server.h"
#include "sio_thread.h"

#define SIO_SERVERS_MAX_THREADS 255

#define SIO_SERVER_THREADS_RATIO 16


struct sio_servers
{
    struct sio_server *serv;
    struct sio_thread *worthrs[SIO_SERVERS_MAX_THREADS];
};


#define SIO_SERVERS_THREADS_CREATE(thread, ops, count, ret) \
    do { \
        for (int i = 0; i < count; i++) { \
            thread[i] = ops; \
            if (thread[i] == NULL) { \
                ret = -1; \
                break; \
            } \
        } \
    } while (0);

#define SIO_SERVERS_THREADS_START(thread, count, ret) \
    do { \
        for (int i = 0; i < count; i++) { \
            if (thread[i] != NULL) { \
                ret = sio_thread_start(thread[i]); \
                if (ret == -1) { \
                    break; \
                } \
            } \
        } \
    } while (0);

#define SIO_SERVERS_THREADS_DESTORY(thread, count) \
    do { \
        for (int i = 0; i < count; i++) { \
            if (thread[i]) { \
                sio_thread_destory(thread[i]); \
                thread[i] = NULL; \
            } \
        } \
    } while (0);

static int sio_socket_readable(void *ptr, const char *data, int len)
{
    struct sio_socket *sock = ptr;
    if (len == 0) {
        printf("client socket close\n");
        return 0;
    }

    sio_socket_mplex(sock, SIO_EV_OPT_MOD, SIO_EVENTS_IN | SIO_EVENTS_OUT);

    return 0;
}

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

static int sio_socket_writeable(void *ptr, const char *data, int len)
{
    struct sio_socket *sock = ptr;

    sio_socket_mplex(sock, SIO_EV_OPT_MOD, SIO_EVENTS_IN);
    sio_socket_write(sock, g_resp, strlen(g_resp));
    return 0;
}

static int sio_server_newconn(struct sio_server *serv)
{
    struct sio_socket *sock = sio_socket_create(SIO_SOCK_TCP);
    union sio_socket_opt opt = {
        .ops.read_cb = sio_socket_readable,
        .ops.write_cb = sio_socket_writeable
    };
    sio_socket_setopt(sock, SIO_SOCK_OPS, &opt);

    int ret = sio_server_accept(serv, sock);
    if(ret == -1) {
        sio_socket_destory(sock);
        return ret;
    }

    opt.nonblock = 1;
    sio_socket_setopt(sock, SIO_SOCK_NONBLOCK, &opt);

    sio_server_socket_mplex(serv, sock);

    return ret;
}

static int sio_server_closed(struct sio_server *sock)
{
    printf("server close\n");
    return 0;
}

void *sio_servers_thread_start_routine(void *arg)
{
    return NULL;
}

static inline
int sio_servers_create_threads(struct sio_servers *servs, unsigned char threads)
{
    struct sio_thread **worthrs = servs->worthrs;

    int ret = 0;
    SIO_SERVERS_THREADS_CREATE(worthrs, sio_thread_create(sio_servers_thread_start_routine, servs), threads, ret);
    if (ret == -1) {
        SIO_SERVERS_THREADS_DESTORY(worthrs, threads);
        return -1;
    }

    SIO_SERVERS_THREADS_START(worthrs, threads, ret);
    if (ret == -1) {
        SIO_SERVERS_THREADS_DESTORY(worthrs, threads);
        return -1;
    }

    return 0;
}

static inline
struct sio_server *sio_servers_create_server(enum sio_socket_proto type, unsigned char threads)
{
    struct sio_server *serv = sio_server_create2(type, threads);
    SIO_COND_CHECK_RETURN_VAL(!serv, NULL);

    union sio_server_opt ops = {
        .ops.accept_cb = sio_server_newconn,
        .ops.close_cb = sio_server_closed
    };
    sio_server_setopt(serv, SIO_SERV_OPS, &ops);

    return serv;
}

static inline
struct sio_servers *sio_servers_create_imp(enum sio_socket_proto type, unsigned char servthrs, unsigned char workthrs)
{
    struct sio_servers *servs = malloc(sizeof(struct sio_servers));
    SIO_COND_CHECK_RETURN_VAL(!servs, NULL);

    memset(servs, 0, sizeof(struct sio_servers));

    struct sio_server *serv = sio_servers_create_server(type, servthrs);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!serv, NULL,
        free(servs));

    servs->serv = serv;

    int ret = sio_servers_create_threads(servs, workthrs);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, NULL,
        sio_server_destory(serv),
        free(servs));

    return servs;
}

struct sio_servers *sio_servers_create(enum sio_socket_proto type)
{
    return sio_servers_create_imp(type, 1, 1);
}

struct sio_servers *sio_servers_create2(enum sio_socket_proto type, unsigned char threads)
{
    SIO_COND_CHECK_RETURN_VAL(threads == 0, NULL);

    unsigned char servthrs = threads % SIO_SERVER_THREADS_RATIO;
    servthrs = servthrs == 0 ? 1 : servthrs;
    unsigned char workthrs = threads - servthrs;
    workthrs = workthrs == 0 ? 1 : workthrs;
    return sio_servers_create_imp(type, servthrs, workthrs);
}

int sio_servers_listen(struct sio_servers *servs, struct sio_socket_addr *addr)
{
    SIO_COND_CHECK_RETURN_VAL(!servs || !addr, -1);

    return sio_server_listen(servs->serv, addr);
}

int sio_servers_destory(struct sio_servers *servs)
{
    SIO_COND_CHECK_RETURN_VAL(!servs, -1);

    int ret = sio_server_destory(servs->serv);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    struct sio_thread **worthrs = servs->worthrs;
    SIO_SERVERS_THREADS_DESTORY(worthrs, SIO_SERVERS_MAX_THREADS);

    free(servs);

    return 0;
}