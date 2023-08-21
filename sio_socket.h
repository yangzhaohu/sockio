#ifndef SIO_SOCKET_H_
#define SIO_SOCKET_H_

#include "sio_event.h"

struct sio_socket;
struct sio_mplex;

enum sio_socket_shuthow
{
    SIO_SOCK_SHUTRD = 1,
    SIO_SOCK_SHUTWR = 2,
    SIO_SOCK_SHUTRDWR = SIO_SOCK_SHUTRD | SIO_SOCK_SHUTWR
};

enum sio_socket_proto
{
    SIO_SOCK_TCP,
    SIO_SOCK_UDP
};

struct sio_socket_addr
{
    char addr[48];
    int port;
};

struct sio_socket_ops
{
    int (*readable)(struct sio_socket *sock);
    int (*readfromable)(struct sio_socket *sock);
    int (*writeable)(struct sio_socket *sock);
    int (*writetoable)(struct sio_socket *sock);
    int (*acceptasync)(struct sio_socket *sock, struct sio_socket *newsock);
    int (*readasync)(struct sio_socket *sock, const char *data, int len);
    int (*writeasync)(struct sio_socket *sock, const char *data, int len);
    int (*closeable)(struct sio_socket *sock);
};

enum sio_socket_optcmd
{
    /* set pointer to private data */
    SIO_SOCK_PRIVATE,
    /* set callback ops function */
    SIO_SOCK_OPS,
    /* set socket mplex */
    SIO_SOCK_MPLEX,
    /* set recv buffer size */
    SIO_SOCK_RCVBUF,
    /* set send buffer size */
    SIO_SOCK_SNDBUF,
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
    struct sio_socket_ops ops;
    struct sio_mplex *mplex;
    int rcvbuf;
    int sndbuf;
    int nonblock;
    int reuseaddr;
    int keepalive;
};


#ifdef __cplusplus
extern "C" {
#endif

unsigned int sio_socket_struct_size();

struct sio_socket *sio_socket_create(enum sio_socket_proto proto, char *placement);
struct sio_socket *sio_socket_create2(enum sio_socket_proto proto, char *placement);

int sio_socket_setopt(struct sio_socket *sock, enum sio_socket_optcmd cmd, union sio_socket_opt *opt);
int sio_socket_getopt(struct sio_socket *sock, enum sio_socket_optcmd cmd, union sio_socket_opt *opt);

int sio_socket_peername(struct sio_socket *sock, struct sio_socket_addr *peer);
int sio_socket_sockname(struct sio_socket *sock, struct sio_socket_addr *addr);

int sio_socket_bind(struct sio_socket *sock, struct sio_socket_addr *addr);
int sio_socket_listen(struct sio_socket *sock, struct sio_socket_addr *addr);

int sio_socket_accept_has_pend(struct sio_socket *sock);
int sio_socket_accept(struct sio_socket *sock, struct sio_socket *newsock);

int sio_socket_async_accept(struct sio_socket *serv, struct sio_socket *sock);

int sio_socket_connect(struct sio_socket *sock, struct sio_socket_addr *addr);

int sio_socket_read(struct sio_socket *sock, char *buf, int len);
int sio_socket_readfrom(struct sio_socket *sock, char *buf, int len, struct sio_socket_addr *peer);
int sio_socket_async_read(struct sio_socket *sock, char *buf, int len);

int sio_socket_write(struct sio_socket *sock, const char *buf, int len);
int sio_socket_writeto(struct sio_socket *sock, const char *buf, int len, struct sio_socket_addr *peer);
int sio_socket_async_write(struct sio_socket *sock, char *buf, int len);

int sio_socket_mplex(struct sio_socket *sock, enum sio_events_opt op, enum sio_events events);

int sio_socket_shutdown(struct sio_socket *sock, enum sio_socket_shuthow how);
int sio_socket_close(struct sio_socket *sock);
int sio_socket_destory(struct sio_socket *sock);

#ifdef __cplusplus
}
#endif

#endif