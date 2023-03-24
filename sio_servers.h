#ifndef SIO_SERVERS_H_
#define SIO_SERVERS_H_

#include "sio_socket.h"

struct sio_servers;

enum sio_servers_optcmd
{
    SIO_SERVS_OPS
};

struct sio_servers_ops
{
    int (*new_conn)(struct sio_socket *sock);
    int (*data_ready)(struct sio_socket *sock, const char *data, int len);
    int (*conn_closed)(struct sio_socket *sock);
};

union sio_servers_opt
{
    struct sio_servers_ops ops;
};

#ifdef __cplusplus
extern "C" {
#endif

struct sio_servers *sio_servers_create(enum sio_socket_proto type);

struct sio_servers *sio_servers_create2(enum sio_socket_proto type, unsigned char threads);

int sio_servers_setopt(struct sio_servers *servs, enum sio_servers_optcmd cmd, union sio_servers_opt *opt);

int sio_servers_listen(struct sio_servers *servs, struct sio_socket_addr *addr);

int sio_servers_destory(struct sio_servers *servs);

#ifdef __cplusplus
}
#endif

#endif