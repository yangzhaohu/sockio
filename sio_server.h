#ifndef SIO_SERVER_H_
#define SIO_SERVER_H_

#include "sio_socket.h"

enum sio_server_advcmd
{
    SIO_SERV_THREAD_MODEL
};

#define SIO_SERVER_MPLEX_THREAD_LIMIT 4
#define SIO_SERVER_WORK_THREAD_MULTI_LIMIT 4

struct sio_server_thread_count
{
    /* mplex thread count */
    unsigned short mplex_count:SIO_SERVER_MPLEX_THREAD_LIMIT;
    /* multiples of mplex threads */
    unsigned short work_multi:SIO_SERVER_WORK_THREAD_MULTI_LIMIT;
};

union sio_server_adv
{
    struct sio_server_thread_count thr_count;
};

struct sio_server;

#ifdef __cplusplus
extern "C" {
#endif

struct sio_server *sio_server_create(enum sio_socket_proto type, struct sio_server_thread_count *thrc);

int sio_server_advance(struct sio_server *serv, enum sio_server_advcmd cmd, union sio_server_adv *adv);

int sio_server_listen(struct sio_server *serv, struct sio_socket_addr *addr);

#ifdef __cplusplus
}
#endif

#endif