#ifndef SIO_SERVMOD_H_
#define SIO_SERVMOD_H_

struct sio_servmod;

enum sio_servmod_type
{
    SIO_SERVMOD_RAW = 1,
    SIO_SERVMOD_HTTP,
    SIO_SERVMOD_BUTT
};

struct sio_mod_ins
{
    int (*mod_version)(const char **version);
    int (*mod_type)(void);

    int (*install)(void **mod);
    int (*stream_in)(void *mod, const char *data, int len);
    int (*unstall)(void *mod);
};

#ifdef __cplusplus
extern "C" {
#endif

struct sio_servmod *sio_servmod_create(enum sio_servmod_type type);

int sio_servmod_loadmod(struct sio_servmod *servmod, struct sio_mod_ins *ops);

int sio_servmod_destory(struct sio_servmod *servmod);

#ifdef __cplusplus
}
#endif

#endif