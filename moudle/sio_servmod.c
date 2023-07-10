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

    union sio_servmod_opt opt;
};

struct sio_mod_ins g_global_mod[SIO_SUBMOD_BUTT] = {
    [SIO_SUBMOD_HTTP] = {
        .mod_name = sio_httpmod_name,
        .mod_version = sio_httpmod_version,
        .mod_type = sio_httpmod_type,
        .install = sio_httpmod_create,
        .mod_hook = sio_httpmod_hookmod,
        .unstall = sio_httpmod_destory,
        .stream_in = sio_httpmod_streamin,
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
        mmod->ins->stream_in(mmod->mod, sock, data, len);
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

struct sio_servmod *sio_servmod_create(enum sio_submod_type type)
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
    servmod->serv = serv;

    // module init
    struct sio_servmod_mod *mmod = &servmod->mmod;
    mmod->ins = &g_global_mod[type];
    void *mod = NULL;
    mmod->ins->install(&mod);
    mmod->mod = mod;
    
    return servmod;
}

static inline
int sio_servmod_set_addr(struct sio_servmod *servmod, struct sio_servmod_addr *addr)
{
    union sio_servmod_opt *opt = &servmod->opt;
    if (strlen(addr->addr) >= 32) {
        return -1;
    }

    memcpy(opt->addr.addr, addr->addr, strlen(addr->addr));
    opt->addr.port = addr->port;

    return 0;
}

int sio_servmod_setopt(struct sio_servmod *servmod, enum sio_servmod_optcmd cmd, union sio_servmod_opt *opt)
{
    SIO_COND_CHECK_RETURN_VAL(!servmod || !opt, -1);

    int ret = 0;
    switch (cmd)
    {
    case SIO_SERVMOD_ADDR:
        ret = sio_servmod_set_addr(servmod, &opt->addr);
        break;
        break;
    
    default:
        ret = -1;
        break;
    }

    return ret;
}

int sio_servmod_getopt(struct sio_servmod *servmod, enum sio_servmod_optcmd cmd, union sio_servmod_opt *opt)
{
    return -1;
}

int sio_servmod_dowork(struct sio_servmod *servmod)
{
    SIO_COND_CHECK_RETURN_VAL(!servmod, -1);

    union sio_servmod_opt *opt = &servmod->opt;
    if (strlen(opt->addr.addr) < 0 || opt->addr.port <= 0) {
        return -1;
    }

    struct sio_socket_addr addr = { 0 };
    memcpy(addr.addr, opt->addr.addr, strlen(opt->addr.addr));
    addr.port = opt->addr.port;

    int ret = sio_server_listen(servmod->serv, &addr);

    return ret;
}

int sio_servmod_loadmod(struct sio_servmod *servmod, struct sio_mod_ins *ops)
{
    return 0;
}

int sio_servmod_destory(struct sio_servmod *servmod)
{
    SIO_COND_CHECK_RETURN_VAL(!servmod, -1);

    struct sio_server *serv = servmod->serv;
    sio_server_destory(serv);

    struct sio_servmod_mod *mmod = &servmod->mmod;
    mmod->ins->unstall(mmod->mod);
    mmod->mod = NULL;

    return 0;
}