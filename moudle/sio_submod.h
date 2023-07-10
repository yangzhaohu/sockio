#ifndef SIO_SUBMOD_H_
#define SIO_SUBMOD_H_

typedef void * sio_conn_t;

struct sio_mod_ins
{
    int (*mod_version)(const char **version);
    int (*mod_type)(void);

    int (*install)(void **mod);
    int (*mod_hook)(void *mod, struct sio_mod_ins *ins);
    int (*unstall)(void *mod);

    int (*stream_conn)(void *mod, sio_conn_t conn);
    int (*stream_in)(void *mod, sio_conn_t conn, const char *data, int len);
    int (*stream_close)(void *mod, sio_conn_t conn);
};

struct sio_mod_ops
{
    int a;
};

#endif