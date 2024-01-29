#ifndef SIO_SERVER_H_
#define SIO_SERVER_H_

#include "sio_socket.h"

struct sio_server;

enum sio_servio
{
    SIO_SERV_NIO,
    SIO_SERV_AIO
};

enum sio_servopc
{
    SIO_SERV_PRIVATE,
    SIO_SERV_OPS,
    SIO_SERV_SSL_CACERT,
    SIO_SERV_SSL_USERCERT,
    SIO_SERV_SSL_USERKEY,
    SIO_SERV_SSL_VERIFY_PEER
};

struct sio_servops
{
    int (*newconnection)(struct sio_server *serv);
};

union sio_servopt
{
    void *private;
    struct sio_servops ops;
    const char *data;
    unsigned char enable;
};

#ifdef __cplusplus
extern "C" {
#endif

int sio_server_set_default(enum sio_servio mode);

struct sio_server *sio_server_create(enum sio_sockprot prot);

struct sio_server *sio_server_create2(enum sio_sockprot prot, unsigned char threads);

int sio_server_setopt(struct sio_server *serv, enum sio_servopc cmd, union sio_servopt *opt);
int sio_server_getopt(struct sio_server *serv, enum sio_servopc cmd, union sio_servopt *opt);

int sio_server_listen(struct sio_server *serv, struct sio_sockaddr *addr);

int sio_server_accept(struct sio_server *serv, struct sio_socket **sock);

int sio_server_shutdown(struct sio_server *serv);

int sio_server_destory(struct sio_server *serv);

#ifdef __cplusplus
}
#endif

#endif