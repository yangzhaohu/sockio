#ifndef SIO_HTTPPROT_H_
#define SIO_HTTPPROT_H_

#include "sio_def.h"

struct sio_httpprot;

enum sio_httpprot_type
{
    SIO_HTTP_REQUEST,
    SIO_HTTP_RESPONSE
};

enum sio_httpprot_stat
{
    SIO_HTTPPROTO_STAT_BEGIN,
    SIO_HTTPPROTO_STAT_HEADCOMPLETE,
    SIO_HTTPPROTO_STAT_CHUNKCOMPLETE,
    SIO_HTTPPROTO_STAT_COMPLETE
};

enum sio_httpprot_data
{
    SIO_HTTPPROTO_DATA_URL,
    SIO_HTTPPROTO_DATA_STATUS,
    SIO_HTTPPROTO_DATA_CHUNKHEAD,
    SIO_HTTPPROTO_DATA_BODY
};

enum sio_httpprot_optcmd
{
    SIO_HTTPPROT_PRIVATE,
    SIO_HTTPPROT_OPS
};

struct sio_httpprot_ops
{
    int (*prot_stat)(void *handler, enum sio_httpprot_stat type);
    int (*prot_field)(void *handler, const sio_str_t *field, const sio_str_t *val);
    int (*prot_data)(void *handler, enum sio_httpprot_data type, const sio_str_t *data);
    int (*prot_over)(void *handler);
};

union sio_httpprot_opt
{
    void *private;
    struct sio_httpprot_ops ops;
};

#ifdef __cplusplus
extern "C" {
#endif

struct sio_httpprot *sio_httpprot_create(enum sio_httpprot_type type);

int sio_httpprot_setopt(struct sio_httpprot *httpprot, enum sio_httpprot_optcmd cmd, union sio_httpprot_opt *opt);

int sio_httpprot_process(struct sio_httpprot *httpprot, const char *data, unsigned int len);

int sio_httpprot_destory(struct sio_httpprot *httpprot);

#ifdef __cplusplus
}
#endif

#endif