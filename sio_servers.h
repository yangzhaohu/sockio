#ifndef SIO_SERVERS_H_
#define SIO_SERVERS_H_

#include "sio_socket.h"

struct sio_servers;


#ifdef __cplusplus
extern "C" {
#endif

struct sio_servers *sio_servers_create(enum sio_socket_proto type);

struct sio_servers *sio_servers_create2(enum sio_socket_proto type, unsigned char threads);

int sio_servers_listen(struct sio_servers *servs, struct sio_socket_addr *addr);

int sio_servers_destory(struct sio_servers *servs);

#ifdef __cplusplus
}
#endif

#endif