#ifndef SIO_SONN_H_
#define SIO_SONN_H_

#include "sio_socket.h"

struct sio_conn;
typedef struct sio_conn* sio_conn_t;

struct sio_server;

enum sio_conn_optcmd
{
    SIO_CONN_SERVER,
    SIO_CONN_PRIVATE
};

union sio_conn_opt
{
    struct sio_server *server;
    void *private;
};

#ifdef __cplusplus
extern "C" {
#endif

unsigned int sio_conn_struct_size();

sio_conn_t sio_conn_create(enum sio_socket_proto proto, void *placement);

struct sio_socket *sio_conn_socket_ref(sio_conn_t conn);

sio_conn_t sio_conn_ref_socket_conn(struct sio_socket *sock);

int sio_conn_setopt(sio_conn_t conn, enum sio_conn_optcmd cmd, union sio_conn_opt *opt);

int sio_conn_getopt(sio_conn_t conn, enum sio_conn_optcmd cmd, union sio_conn_opt *opt);

int sio_conn_write(sio_conn_t conn, const char *buf, int len);

int sio_conn_close(sio_conn_t conn);

int sio_conn_destory(sio_conn_t conn);

#ifdef __cplusplus
}
#endif

#endif