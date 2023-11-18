#ifndef SIO_SERVER_H_
#define SIO_SERVER_H_

#include "sio_socket.h"

struct sio_server;

enum sio_servoptc
{
    SIO_SERV_PRIVATE,
    SIO_SERV_OPS
};

struct sio_servops
{
    int (*accept)(struct sio_server *serv);
};

union sio_servopt
{
    void *private;
    struct sio_servops ops;
};

#ifdef __cplusplus
extern "C" {
#endif

struct sio_server *sio_server_create(enum sio_sockprot type);

struct sio_server *sio_server_create2(enum sio_sockprot type, unsigned char threads);

int sio_server_setopt(struct sio_server *serv, enum sio_servoptc cmd, union sio_servopt *opt);
int sio_server_getopt(struct sio_server *serv, enum sio_servoptc cmd, union sio_servopt *opt);

int sio_server_listen(struct sio_server *serv, struct sio_sockaddr *addr);

int sio_server_accept(struct sio_server *serv, struct sio_socket *sock);

int sio_server_socket_mplex(struct sio_server *serv, struct sio_socket *sock);

int sio_server_shutdown(struct sio_server *serv);

int sio_server_destory(struct sio_server *serv);

#ifdef __cplusplus
}
#endif

#endif