#include "sio_httpmod.h"
#include <string.h>
#include <stdlib.h>
#include "sio_common.h"
#include "proto/sio_httpprot.h"
#include "sio_log.h"

struct sio_httpmod
{
    struct sio_httpprot *httpprot;
};

int sio_httpmod_name(const char **name)
{
    *name = "sio_http_core_module";
    return 0;
}

int sio_httpmod_type()
{
    return SIO_SUBMOD_HTTP;
}

int sio_httpmod_version(const char **version)
{
    *version = "0.0.1";
    return 0;
}

int sio_httpmod_protodata(void *handler, enum sio_httpprot_valtype valtype, const char *data, int len)
{
    SIO_LOGE("%.*s\n", (int)len, data);
    return 0;
}

int sio_httpmod_create(void **mod)
{
    struct sio_httpmod *httpmod = malloc(sizeof(struct sio_httpmod));
    SIO_COND_CHECK_RETURN_VAL(!httpmod, -1)

    struct sio_httpprot *httpprot = sio_httpprot_create(SIO_HTTP_REQUEST);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!httpprot, -1,
        free(httpmod));
    
    union sio_httpprot_opt opt = {
        .private = httpmod
    };
    sio_httpprot_setopt(httpprot, SIO_HTTPPROT_PRIVATE, &opt);

    opt.ops.prot_data = sio_httpmod_protodata;
    sio_httpprot_setopt(httpprot, SIO_HTTPPROT_OPS, &opt);
    
    httpmod->httpprot = httpprot;

    *mod = httpmod;

    return 0;
}

int sio_httpmod_streamin(void *mod, sio_conn_t conn, const char *data, int len)
{
    struct sio_httpprot *httpprot = ((struct sio_httpmod *)mod)->httpprot;
    sio_httpprot_process(httpprot, data, len);
    return 0;
}

int sio_httpmod_hookmod(void *mod, struct sio_mod_ins *ins)
{
    return 0;
}

int sio_httpmod_destory(void *mod)
{
    free(mod);

    return 0;
}