#include "sio_service.h"
#include <string.h>
#include <stdlib.h>
#include "sio_server.h"
#include "sio_common.h"
#include "moudle/http/sio_httpmod.h"
#include "moudle/rtsp/sio_rtspmod.h"
#include "moudle/doip/sio_doipmod.h"
#include "sio_log.h"

struct sio_service
{
    struct sio_server *serv;
    struct sio_submod submod;

    union sio_service_opt opt;
};

struct sio_submod g_global_mod[SIO_SUBMOD_BUTT] = {
    [SIO_SUBMOD_HTTP] = {
        .mod_name = sio_httpmod_name,
        .mod_version = sio_httpmod_version,
        .mod_type = sio_httpmod_type,
        .install = sio_httpmod_create,
        .modhook = sio_httpmod_modhook,
        .setlocate = sio_httpmod_setlocate,
        .newconn = sio_httpmod_newconn,
        .unstall = sio_httpmod_destory
    },
    [SIO_SUBMOD_RTSP] = {
        .mod_name = sio_rtspmod_name,
        .mod_version = sio_rtspmod_version,
        .mod_type = sio_rtspmod_type,
        .install = sio_rtspmod_create,
        .setlocate = sio_rtspmod_setlocate,
        .newconn = sio_rtspmod_newconn,
        .unstall = sio_rtspmod_destory
    },
    [SIO_SUBMOD_DOIP] = {
        .mod_name = sio_doipmod_name,
        .mod_version = sio_doipmod_version,
        .mod_type = sio_doipmod_type,
        .install = sio_doipmod_create,
        .setlocate = sio_doipmod_setlocate,
        .newconn = sio_doipmod_newconn,
        .unstall = sio_doipmod_destory
    }
 };

static inline
int sio_service_newconn(struct sio_server *serv)
{
    union sio_server_opt opts = { 0 };
    sio_server_getopt(serv, SIO_SERV_PRIVATE, &opts);

    struct sio_service *service = opts.private;
    struct sio_submod *submod = &service->submod;

    if (submod->newconn) {
        submod->newconn(submod->mod, serv);
    }

    return 0;
}

static inline
struct sio_server *sio_service_createserv()
{
    struct sio_server *serv = sio_server_create(SIO_SOCK_TCP);
    SIO_COND_CHECK_RETURN_VAL(!serv, NULL);

    union sio_server_opt ops = {
        .ops.accept = sio_service_newconn
    };
    sio_server_setopt(serv, SIO_SERV_OPS, &ops);

    return serv;
}

static inline
enum sio_submod_type sio_service_modtype(enum sio_service_type type)
{
    enum sio_submod_type modtype;
    switch (type)
    {
    case SIO_SERVICE_HTTP:
        modtype = SIO_SUBMOD_HTTP;
        break;
    case SIO_SERVICE_RTSP:
        modtype = SIO_SUBMOD_RTSP;
        break;
    case SIO_SERVICE_DOIP:
        modtype = SIO_SUBMOD_DOIP;
        break;

    default:
        modtype = SIO_SUBMOD_BUTT;
        break;
    }

    return modtype;
}

struct sio_service *sio_service_create(enum sio_service_type type)
{
    enum sio_submod_type modtype = sio_service_modtype(type);
    SIO_COND_CHECK_RETURN_VAL(modtype == SIO_SUBMOD_BUTT, NULL);

    struct sio_service *service = malloc(sizeof(struct sio_service));
    SIO_COND_CHECK_RETURN_VAL(!service, NULL);

    memset(service, 0, sizeof(struct sio_service));

    // server init
    struct sio_server *serv = sio_service_createserv();
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!serv, NULL,
        free(service));
    
    union sio_server_opt ops = {
        .private = service
    };
    sio_server_setopt(serv, SIO_SERV_PRIVATE, &ops);
    service->serv = serv;

    // module init
    service->submod = g_global_mod[modtype];
    struct sio_submod *submod = &service->submod;
    if (submod->install) {
        submod->mod = submod->install();
        SIO_COND_CHECK_CALLOPS_RETURN_VAL(submod->mod == NULL, NULL,
            sio_server_destory(serv),
            free(service));
    } else {
        sio_server_destory(serv);
        free(service);
        service = NULL;
    }
    
    return service;
}

static inline
int sio_service_set_addr(struct sio_service *service, struct sio_service_addr *addr)
{
    union sio_service_opt *opt = &service->opt;
    if (strlen(addr->addr) >= 32) {
        return -1;
    }

    memcpy(opt->addr.addr, addr->addr, strlen(addr->addr));
    opt->addr.port = addr->port;

    return 0;
}

int sio_service_setopt(struct sio_service *service, enum sio_service_optcmd cmd, union sio_service_opt *opt)
{
    SIO_COND_CHECK_RETURN_VAL(!service || !opt, -1);

    int ret = 0;
    switch (cmd)
    {
    case SIO_SERVICE_ADDR:
        ret = sio_service_set_addr(service, &opt->addr);
        break;
        break;
    
    default:
        ret = -1;
        break;
    }

    return ret;
}

int sio_service_getopt(struct sio_service *service, enum sio_service_optcmd cmd, union sio_service_opt *opt)
{
    return -1;
}

int sio_service_setlocat(struct sio_service *service, const struct sio_location *locations, int size)
{
    SIO_COND_CHECK_RETURN_VAL(!service, -1);

    struct sio_submod *submod = &service->submod;

    int ret = -1;
    if (submod->setlocate) {
        ret = submod->setlocate(submod->mod, locations, size);
    }

    return ret;
}

int sio_service_dowork(struct sio_service *service)
{
    SIO_COND_CHECK_RETURN_VAL(!service, -1);

    union sio_service_opt *opt = &service->opt;
    if (strlen(opt->addr.addr) < 0 || opt->addr.port <= 0) {
        return -1;
    }

    struct sio_socket_addr addr = { 0 };
    memcpy(addr.addr, opt->addr.addr, strlen(opt->addr.addr));
    addr.port = opt->addr.port;

    int ret = sio_server_listen(service->serv, &addr);

    return ret;
}

int sio_service_insmod(struct sio_service *service, const char *modname, struct sio_submod *mod)
{
    return 0;
}

int sio_service_destory(struct sio_service *service)
{
    SIO_COND_CHECK_RETURN_VAL(!service, -1);

    struct sio_server *serv = service->serv;
    sio_server_destory(serv);

    struct sio_submod *submod = &service->submod;
    if (submod->unstall) {
        submod->unstall(submod->mod);
    }

    return 0;
}