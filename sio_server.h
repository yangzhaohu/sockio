#ifndef SIO_SERVER_H_
#define SIO_SERVER_H_

#include "sio_socket.h"

enum sio_server_advcmd
{
    SIO_SERV_THREAD_MODEL
};

#define SIO_SERVER_MPLEX_THREAD_LIMIT 4
#define SIO_SERVER_WORK_THREAD_MULTI_LIMIT 4

struct sio_server_thread_mask
{
    unsigned int mthrs: 4;
    unsigned int wthrs: 8;
};

union sio_server_adv
{
    struct sio_server_thread_mask tmask;
};

struct sio_server;

#ifdef __cplusplus
extern "C" {
#endif

struct sio_server *sio_server_create(enum sio_socket_proto type);

struct sio_server *sio_server_create2(enum sio_socket_proto type, struct sio_server_thread_mask tmask);

int sio_server_advance(struct sio_server *serv, enum sio_server_advcmd cmd, union sio_server_adv *adv);

int sio_server_listen(struct sio_server *serv, struct sio_socket_addr *addr);

#ifdef __cplusplus
}
#endif

#endif