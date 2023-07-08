#include "sio_servmod.h"
#include <string.h>
#include <stdlib.h>
#include "sio_server.h"
#include "sio_common.h"
#include "sio_httpmod.h"
#include "sio_log.h"

struct sio_servmod_mod
{
    void *mod;
    struct sio_mod_ins *ins;
};

struct sio_servmod
{
    struct sio_server *serv;
    struct sio_servmod_mod mmod;
};

struct sio_mod_ins g_global_mod[SIO_SERVMOD_BUTT] = {
    [SIO_SERVMOD_HTTP] = {
        .mod_version = sio_httpmod_version,
        .mod_type = sio_httpmod_type,
        .install = sio_httpmod_create,
        .stream_in = sio_httpmod_streamin,
        .unstall = sio_httpmod_destory
    }
 };

static inline
int sio_socket_readable(struct sio_socket *sock)
{
    union sio_socket_opt opt = { 0 };
    sio_socket_getopt(sock, SIO_SOCK_PRIVATE, &opt);

    struct sio_servmod *servmod = opt.private;

    struct sio_servmod_mod *mmod = &servmod->mmod;
    char data[512] = { 0 };
    int len = sio_socket_read(sock, data, 512);
    if (len > 0) {
        mmod->ins->stream_in(mmod->mod, data, len);
    }

    return 0;
}

static inline
int sio_socket_writeable(struct sio_socket *sock)
{
    return 0;
}

static inline
int sio_socket_closeable(struct sio_socket *sock)
{
    return 0;
}

static inline
struct sio_socket *sio_servmod_socket()
{
    struct sio_socket *sock = sio_socket_create(SIO_SOCK_TCP, NULL);
    union sio_socket_opt opt = {
        .ops.readable = sio_socket_readable,
        .ops.writeable = sio_socket_writeable,
        .ops.closeable = sio_socket_closeable
    };
    sio_socket_setopt(sock, SIO_SOCK_OPS, &opt);

    opt.nonblock = 1;
    sio_socket_setopt(sock, SIO_SOCK_NONBLOCK, &opt);

    return sock;
}

static inline
int sio_servset_newconn(struct sio_server *serv)
{
    struct sio_socket *sock = sio_servmod_socket();

    union sio_server_opt opts = { 0 };
    sio_server_getopt(serv, SIO_SERV_PRIVATE, &opts);

    union sio_socket_opt opt = {
        .private = opts.private
    };
    sio_socket_setopt(sock, SIO_SOCK_PRIVATE, &opt);

    sio_server_accept(serv, sock);
    sio_server_socket_mplex(serv, sock);

    return 0;
}

static inline
struct sio_server *sio_servmod_createserv()
{
    struct sio_server *serv = sio_server_create(SIO_SOCK_TCP);
    SIO_COND_CHECK_RETURN_VAL(!serv, NULL);

    union sio_server_opt ops = {
        .ops.accept = sio_servset_newconn
    };
    sio_server_setopt(serv, SIO_SERV_OPS, &ops);

    return serv;
}

struct sio_servmod *sio_servmod_create(enum sio_servmod_type type)
{
    struct sio_servmod *servmod = malloc(sizeof(struct sio_servmod));
    SIO_COND_CHECK_RETURN_VAL(!servmod, NULL);

    memset(servmod, 0, sizeof(struct sio_servmod));

    // server init
    struct sio_server *serv = sio_servmod_createserv();
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!serv, NULL,
        free(servmod));
    
    union sio_server_opt ops = {
        .private = servmod
    };
    sio_server_setopt(serv, SIO_SERV_PRIVATE, &ops);

    // module init
    struct sio_servmod_mod *mmod = &servmod->mmod;
    mmod->ins = &g_global_mod[type];
    void *mod = NULL;
    mmod->ins->install(&mod);
    mmod->mod = mod;

    struct sio_socket_addr addr = {"127.0.0.1", 8000};
    sio_server_listen(serv, &addr);
    
    return servmod;
}

int sio_servmod_loadmod(struct sio_servmod *servmod, struct sio_mod_ins *ops)
{
    return 0;
}

int sio_servmod_destory(struct sio_servmod *servmod)
{
    struct sio_servmod_mod *mmod = &servmod->mmod;
    mmod->ins->unstall(mmod->mod);
    mmod->mod = NULL;

    return 0;
}