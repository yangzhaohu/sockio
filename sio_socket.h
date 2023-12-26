#ifndef SIO_SOCKET_H_
#define SIO_SOCKET_H_

#include "sio_event.h"

struct sio_socket;
struct sio_mplex;

enum sio_socksh
{
    SIO_SOCK_SHUTRD = 1,
    SIO_SOCK_SHUTWR = 2,
    SIO_SOCK_SHUTRDWR = SIO_SOCK_SHUTRD | SIO_SOCK_SHUTWR
};

enum sio_sockprot
{
    SIO_SOCK_TCP,
    SIO_SOCK_UDP,
    SIO_SOCK_SSL
};

enum sio_sockwhat
{
    SIO_SOCK_NONE,
    SIO_SOCK_OPEN,
    SIO_SOCK_CONNECT,
    SIO_SOCK_HANDSHAKE,
    SIO_SOCK_LISTEN,
    SIO_SOCK_ESTABLISHED,
    SIO_SOCK_CLOSE
};

struct sio_sockaddr
{
    char addr[48];
    int port;
};

struct sio_sockops
{
    int (*connected)(struct sio_socket *sock, enum sio_sockwhat what);
    int (*handshaked)(struct sio_socket *sock);
    int (*readable)(struct sio_socket *sock);
    int (*writeable)(struct sio_socket *sock);
    int (*acceptasync)(struct sio_socket *serv, struct sio_socket *sock);
    int (*readasync)(struct sio_socket *sock, const char *data, int len);
    int (*writeasync)(struct sio_socket *sock, const char *data, int len);
    int (*closeable)(struct sio_socket *sock);
};

enum sio_sockopc
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
    SIO_SOCK_KEEPALIVE,
    /* set socket send timeout */
    SIO_SOCK_SNDTIMEO,
    /* set socket recv timeout, the socket will become non blocking */
    SIO_SOCK_RCVTIMEO,
    SIO_SOCK_SSL_CACERT,
    SIO_SOCK_SSL_USERCERT,
    SIO_SOCK_SSL_USERKEY,
    SIO_SOCK_SSL_VERIFY_PEER
};

union sio_sockopt
{
    void *private;
    struct sio_sockops ops;
    struct sio_mplex *mplex;
    int rcvbuf;
    int sndbuf;
    int nonblock;
    int reuseaddr;
    int keepalive;
    int timeout; // ms
    const char *data;
    unsigned char enable;
};


#ifdef __cplusplus
extern "C" {
#endif

unsigned int sio_socket_struct_size();

struct sio_socket *sio_socket_create(enum sio_sockprot prot, char *placement);
struct sio_socket *sio_socket_create2(enum sio_sockprot prot, char *placement);

struct sio_socket *sio_socket_dup(struct sio_socket *sock, char *placement);
struct sio_socket *sio_socket_dup2(struct sio_socket *sock, char *placement);

int sio_socket_setopt(struct sio_socket *sock, enum sio_sockopc cmd, union sio_sockopt *opt);
int sio_socket_getopt(struct sio_socket *sock, enum sio_sockopc cmd, union sio_sockopt *opt);

int sio_socket_peername(struct sio_socket *sock, struct sio_sockaddr *peer);
int sio_socket_sockname(struct sio_socket *sock, struct sio_sockaddr *addr);

int sio_socket_bind(struct sio_socket *sock, struct sio_sockaddr *addr);
int sio_socket_listen(struct sio_socket *sock, struct sio_sockaddr *addr);

int sio_socket_accept_has_pend(struct sio_socket *sock);
int sio_socket_accept(struct sio_socket *sock, struct sio_socket *newsock);

int sio_socket_async_accept(struct sio_socket *serv, struct sio_socket *sock);

int sio_socket_connect(struct sio_socket *sock, struct sio_sockaddr *addr);

int sio_socket_read(struct sio_socket *sock, char *buf, int len);
int sio_socket_readfrom(struct sio_socket *sock, char *buf, int len, struct sio_sockaddr *peer);
int sio_socket_async_read(struct sio_socket *sock, char *buf, int len);

int sio_socket_write(struct sio_socket *sock, const char *buf, int len);
int sio_socket_writeto(struct sio_socket *sock, const char *buf, int len, struct sio_sockaddr *peer);
int sio_socket_async_write(struct sio_socket *sock, char *buf, int len);

int sio_socket_mplex(struct sio_socket *sock, enum sio_events_opt op, enum sio_events events);

int sio_socket_shutdown(struct sio_socket *sock, enum sio_socksh how);
int sio_socket_close(struct sio_socket *sock);
int sio_socket_destory(struct sio_socket *sock);

#ifdef __cplusplus
}
#endif

#endif