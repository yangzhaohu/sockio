#include "sio_servmod.h"
#include <string.h>
#include <stdlib.h>
#include "sio_server.h"
#include "sio_common.h"
#include "sio_mod.h"
#include "moudle/sio_conn.h"
#include "moudle/http/sio_httpmod.h"
#include "moudle/rtsp/sio_rtspmod.h"
#include "sio_log.h"

struct sio_servmod
{
    struct sio_server *serv;
    struct sio_mod *mod;

    union sio_servmod_opt opt;
};

struct sio_mod g_global_mod[SIO_SUBMOD_BUTT] = {
    [SIO_SUBMOD_HTTP] = {
        .submod = {
            .mod_name = sio_httpmod_name,
            .mod_version = sio_httpmod_version,
            .mod_type = sio_httpmod_type,
            .install = sio_httpmod_create,
            .mod_hook = sio_httpmod_hookmod,
            .unstall = sio_httpmod_destory,
            .stream_conn = sio_httpmod_streamconn,
            .stream_close = sio_httpmod_streamclose,
        },
        .setlocat = sio_httpmod_setlocat,
        .stream_in = sio_httpmod_streamin
    },
    [SIO_SUBMOD_RTSP] = {
        .submod = {
            .mod_name = sio_rtspmod_name,
            .mod_version = sio_rtspmod_version,
            .mod_type = sio_rtspmod_type,
            .install = sio_rtspmod_create,
            .unstall = sio_rtspmod_destory,
            .stream_conn = sio_rtspmod_streamconn,
            .stream_close = sio_rtspmod_streamclose,
        },
        .stream_in = sio_rtspmod_streamin
    }
 };


static inline
int sio_socket_readable(struct sio_socket *sock)
{
    union sio_sockopt opt = { 0 };
    sio_socket_getopt(sock, SIO_SOCK_PRIVATE, &opt);

    struct sio_servmod *servmod = opt.private;

    struct sio_mod *mod = servmod->mod;

    char data[512] = { 0 };
    int len = sio_socket_read(sock, data, 512);
    if (len > 0 && mod->stream_in) {
        sio_conn_t conn = sio_conn_ref_socket_conn(sock);
        mod->stream_in(conn, data, len);
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
    union sio_sockopt opt = { 0 };
    sio_socket_getopt(sock, SIO_SOCK_PRIVATE, &opt);

    struct sio_servmod *servmod = opt.private;
    struct sio_mod *mod = servmod->mod;
    struct sio_submod *submod = &mod->submod;

    sio_conn_t conn = sio_conn_ref_socket_conn(sock);

    if (submod->stream_close) {
        submod->stream_close(conn);
    }
    
    sio_conn_destory(conn);

    return 0;
}

static inline
struct sio_socket *sio_servmod_conn_socket(struct sio_server *serv)
{
    sio_conn_t conn = sio_conn_create(SIO_SOCK_TCP, NULL);
    SIO_COND_CHECK_RETURN_VAL(!conn, NULL);
    union sio_conn_opt optc = { .server = serv };
    sio_conn_setopt(conn, SIO_CONN_SERVER, &optc);

    struct sio_socket *sock = sio_conn_socket_ref(conn);
    union sio_sockopt opt = {
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
int sio_servmod_newconn(struct sio_server *serv)
{
    struct sio_socket *sock = sio_servmod_conn_socket(serv);

    union sio_servopt opts = { 0 };
    sio_server_getopt(serv, SIO_SERV_PRIVATE, &opts);

    struct sio_servmod *servmod = opts.private;
    struct sio_mod *mod = servmod->mod;
    struct sio_submod *submod = &mod->submod;

    union sio_sockopt opt = {
        .private = servmod
    };
    sio_socket_setopt(sock, SIO_SOCK_PRIVATE, &opt);

    int ret = sio_server_accept(serv, sock);
    if (ret == 0 && submod->stream_conn) {
        sio_conn_t conn = sio_conn_ref_socket_conn(sock);
        submod->stream_conn(conn);
    }

    sio_server_socket_mplex(serv, sock);

    return 0;
}

static inline
struct sio_server *sio_servmod_createserv()
{
    struct sio_server *serv = sio_server_create(SIO_SOCK_TCP);
    SIO_COND_CHECK_RETURN_VAL(!serv, NULL);

    union sio_servopt ops = {
        .ops.accept = sio_servmod_newconn
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
    
    union sio_servopt ops = {
        .private = servmod
    };
    sio_server_setopt(serv, SIO_SERV_PRIVATE, &ops);
    servmod->serv = serv;

    // module init
    servmod->mod = &g_global_mod[type];
    struct sio_submod *submod = &servmod->mod->submod;
    if (submod->install) {
        int ret = submod->install();
        SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret != 0, NULL,
            sio_server_destory(serv),
            free(servmod));
    } else {
        sio_server_destory(serv);
        free(servmod);
        servmod = NULL;
    }
    
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

int sio_servmod_setlocat(struct sio_servmod *servmod, const struct sio_location *locations, int size)
{
    SIO_COND_CHECK_RETURN_VAL(!servmod, -1);

    struct sio_mod *mod = servmod->mod;

    int ret = -1;
    if (mod->setlocat) {
        ret = mod->setlocat(locations, size);
    }

    return ret;
}

int sio_servmod_dowork(struct sio_servmod *servmod)
{
    SIO_COND_CHECK_RETURN_VAL(!servmod, -1);

    union sio_servmod_opt *opt = &servmod->opt;
    if (strlen(opt->addr.addr) < 0 || opt->addr.port <= 0) {
        return -1;
    }

    struct sio_sockaddr addr = { 0 };
    memcpy(addr.addr, opt->addr.addr, strlen(opt->addr.addr));
    addr.port = opt->addr.port;

    int ret = sio_server_listen(servmod->serv, &addr);

    return ret;
}

int sio_servmod_insmod(struct sio_servmod *servmod, const char *modname, struct sio_submod *mod)
{
    return 0;
}

int sio_servmod_destory(struct sio_servmod *servmod)
{
    SIO_COND_CHECK_RETURN_VAL(!servmod, -1);

    struct sio_server *serv = servmod->serv;
    sio_server_destory(serv);

    struct sio_mod *mod = servmod->mod;
    struct sio_submod *submod = &mod->submod;
    if (submod->unstall) {
        submod->unstall();
    }

    return 0;
}