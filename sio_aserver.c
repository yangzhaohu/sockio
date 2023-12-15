#include "sio_aserver.h"

struct sio_aserver *sio_aserver_create(enum sio_sockprot type)
{
    return 0;
}

struct sio_aserver *sio_aserver_create2(enum sio_sockprot type, unsigned char threads)
{
    return 0;
}

int sio_aserver_setopt(struct sio_aserver *serv, enum sio_servoptc cmd, union sio_servopt *opt)
{
    return 0;
}

int sio_aserver_getopt(struct sio_aserver *serv, enum sio_servoptc cmd, union sio_servopt *opt)
{
    return 0;
}

int sio_aserver_listen(struct sio_aserver *serv, struct sio_sockaddr *addr)
{
    return 0;
}

int sio_aserver_socket_mplex(struct sio_aserver *serv, struct sio_socket *sock)
{
    return 0;
}

int sio_aserver_shutdown(struct sio_aserver *serv)
{
    return 0;
}

int sio_aserver_destory(struct sio_aserver *serv)
{
    return 0;
}