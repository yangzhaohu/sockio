#ifndef SIO_SUBMOD_H_
#define SIO_SUBMOD_H_

enum sio_submod_type
{
    SIO_SUBMOD_RAW = 1,
    SIO_SUBMOD_HTTP,
    SIO_SUBMOD_RTSP,
    SIO_SUBMOD_DOIP,
    SIO_SUBMOD_BUTT
};

#define SIO_SUBMOD_EXPORTSYMS g_custom_submod
#define SIO_SUBMOD_EXPORTSYMS_INIT(submod) extern const struct sio_submod* SIO_SUBMOD_EXPORTSYMS = &submod

typedef void * sio_submod_t;

struct sio_server;
struct sio_location;

struct sio_submod
{
    sio_submod_t mod;
    int (*mod_name)(const char **name);
    int (*mod_version)(const char **version);
    int (*mod_type)(void);

    sio_submod_t (*install)(void);

    int (*modhook)(sio_submod_t mod, struct sio_submod *submod);
    int (*setlocate)(sio_submod_t mod, const struct sio_location *locations, int size);
    int (*getlocate)(sio_submod_t mod, const char **locations);
    int (*newconn)(sio_submod_t mod, struct sio_server *server);

    int (*unstall)(sio_submod_t mod);
};

#endif