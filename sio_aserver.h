#ifndef SIO_ASERVER_H_
#define SIO_ASERVER_H_

#include "sio_server.h"

struct sio_aserver;

struct sio_aservops
{
    int (*accept)(struct sio_aserver *serv);
};

#ifdef __cplusplus
extern "C" {
#endif

struct sio_aserver *sio_aserver_create(enum sio_sockprot type);

struct sio_aserver *sio_aserver_create2(enum sio_sockprot type, unsigned char threads);

int sio_aserver_setopt(struct sio_aserver *serv, enum sio_servoptc cmd, union sio_servopt *opt);
int sio_aserver_getopt(struct sio_aserver *serv, enum sio_servoptc cmd, union sio_servopt *opt);

int sio_aserver_listen(struct sio_aserver *serv, struct sio_sockaddr *addr);

int sio_aserver_socket_mplex(struct sio_aserver *serv, struct sio_socket *sock);

int sio_aserver_shutdown(struct sio_aserver *serv);

int sio_aserver_destory(struct sio_aserver *serv);

#ifdef __cplusplus
}
#endif

#endif