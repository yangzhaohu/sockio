#ifndef SIO_HTTPPROT_H_
#define SIO_HTTPPROT_H_

struct sio_httpprot;

enum sio_httpprot_type
{
    SIO_HTTP_REQUEST,
    SIO_HTTP_RESPONSE
};

enum sio_httpprot_valtype
{
    SIO_HTTPPROTO_VALTYPE_BEGIN,
    SIO_HTTPPROTO_VALTYPE_URL,
    SIO_HTTPPROTO_VALTYPE_STATUS,
    SIO_HTTPPROTO_VALTYPE_HEADFIELD,
    SIO_HTTPPROTO_VALTYPE_HEADVALUE,
    SIO_HTTPPROTO_VALTYPE_HEADCOMPLETE,
    SIO_HTTPPROTO_VALTYPE_CHUNKHEAD,
    SIO_HTTPPROTO_VALTYPE_CHUNKCOMPLETE,
    SIO_HTTPPROTO_VALTYPE_BODY,
    SIO_HTTPPROTO_VALTYPE_COMPLETE,
};

enum sio_httpprot_optcmd
{
    SIO_HTTPPROT_PRIVATE,
    SIO_HTTPPROT_OPS
};

struct sio_httpprot_ops
{
    int (*prot_data)(void *handler, enum sio_httpprot_valtype type, const char *data, int len);
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