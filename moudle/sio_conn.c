#include "moudle/sio_conn.h"
#include <stdlib.h>
#include <string.h>
#include "sio_common.h"

struct sio_conn_state
{
    unsigned int placement:1;
};

struct sio_conn
{
    struct sio_conn_state stat;
    void *private;
};

#define SIO_CONN_SOCK_MEMPTR(mem) ((void *)mem + sizeof(struct sio_conn))
#define SIO_SOCK_CONN_MEMPTR(mem) ((void *)mem - sizeof(struct sio_conn))

unsigned int sio_conn_struct_size()
{
    return sizeof(struct sio_conn) + sio_socket_struct_size();
}

sio_conn_t sio_conn_create(enum sio_socket_proto proto, void *placement)
{
    int plmt = 0;
    sio_conn_t conn = placement;
    if (conn == NULL) {
        conn = malloc(sio_conn_struct_size());
    } else {
        plmt = 1;
    }

    memset(conn, 0, sizeof(struct sio_conn));

    struct sio_socket *sock = sio_socket_create(proto, SIO_CONN_SOCK_MEMPTR(conn));
    if (!sock) {
        SIO_COND_CHECK_RETURN_VAL(plmt == 1, NULL);
        free(conn);
        return NULL;
    }

    struct sio_conn_state *stat = &conn->stat;
    stat->placement = plmt;

    return conn;
}

struct sio_socket *sio_conn_socket_ref(sio_conn_t conn)
{
    return SIO_CONN_SOCK_MEMPTR(conn);
}

sio_conn_t sio_conn_ref_socket_conn(struct sio_socket *sock)
{
    return SIO_SOCK_CONN_MEMPTR(sock);
}

int sio_conn_setopt(sio_conn_t conn, enum sio_conn_optcmd cmd, union sio_conn_opt *opt)
{
    int ret = 0;
    switch (cmd) {
    case SIO_CONN_PRIVATE:
        conn->private = opt->private;
        break;

    default:
        ret = -1;
        break;
    }

    return ret;
}

int sio_conn_getopt(sio_conn_t conn, enum sio_conn_optcmd cmd, union sio_conn_opt *opt)
{
    int ret = 0;
    switch (cmd) {
    case SIO_CONN_PRIVATE:
        opt->private = conn->private;
        break;

    default:
        ret = -1;
        break;
    }

    return ret;
}

int sio_conn_write(sio_conn_t conn, const char *buf, int len)
{
    struct sio_socket *sock = SIO_CONN_SOCK_MEMPTR(conn);
    return sio_socket_write(sock, buf, len);
}

int sio_conn_close(sio_conn_t conn)
{
    struct sio_socket *sock = SIO_CONN_SOCK_MEMPTR(conn);
    return sio_socket_shutdown(sock, SIO_SOCK_SHUTWR);
}

int sio_conn_destory(sio_conn_t conn)
{
    struct sio_socket *sock = SIO_CONN_SOCK_MEMPTR(conn);
    sio_socket_close(sock);

    struct sio_conn_state *stat = &conn->stat;
    SIO_COND_CHECK_CALLOPS(stat->placement == 0, free(conn));

    return 0;
}