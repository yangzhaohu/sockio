#ifndef SIO_SOCKET_H_
#define SIO_SOCKET_H_

#include "sio_event.h"
#include "sio_io.h"

enum sio_socket_proto
{
    SIO_SOCK_TCP,
    SIO_SOCK_UDP
};

struct sio_socket_addr
{
    char addr[32];
    int port;
};

enum sio_socket_optcmd
{
    /* set pointer to private data */
    SIO_SOCK_PRIVATE,
    /*set callback ops function */
    SIO_SOCK_OPS,
    /* set send/recv buffer size */
    SIO_SOCK_BUFF,
    /* set socket nonblock */
    SIO_SOCK_NONBLOCK,
    /* set socket reuseaddr */
    SIO_SOCK_REUSEADDR,
    /* set socket keepalive */
    SIO_SOCK_KEEPALIVE
};

union sio_socket_opt
{
    void *private;
    struct sio_io_ops ops;

    struct sock_buff {
        int rcvbuf;
        int sndbuf;
    }buff;
    int nonblock;
    int reuseaddr;
    int keepalive;
};

struct sio_socket;
struct sio_mplex;

#ifdef __cplusplus
extern "C" {
#endif

struct sio_socket *sio_socket_create(enum sio_socket_proto proto);
struct sio_socket *sio_socket_create2(enum sio_socket_proto proto);

int sio_socket_option(struct sio_socket *sock, enum sio_socket_optcmd cmd, union sio_socket_opt *opt);

int sio_socket_listen(struct sio_socket *sock, struct sio_socket_addr *addr);

int sio_socket_accept(struct sio_socket *serv, struct sio_socket *sock);
int sio_socket_async_accept(struct sio_socket *serv, struct sio_socket *sock);

int sio_socket_connect(struct sio_socket *sock, struct sio_socket_addr *addr);

int sio_socket_read(struct sio_socket *sock, char *buf, int maxlen);
int sio_socket_async_read(struct sio_socket *sock, char *buf, int maxlen);

int sio_socket_write(struct sio_socket *sock, char *buf, int len);
int sio_socket_async_write(struct sio_socket *sock, char *buf, int len);

int sio_socket_mplex_bind(struct sio_socket *sock, struct sio_mplex *mp);
int sio_socket_mplex(struct sio_socket *sock, enum SIO_EVENTS_OPT op, enum SIO_EVENTS events);

void *sio_socket_private(struct sio_socket *sock);

int sio_socket_close(struct sio_socket *sock);

int sio_socket_destory(struct sio_socket *sock);

#ifdef __cplusplus
}
#endif

#endif