#ifndef SIO_SUBMOD_H_
#define SIO_SUBMOD_H_

#include "sio_server.h"
#include "moudle/sio_locate.h"

enum sio_submod_type
{
    SIO_SUBMOD_RAW = 1,
    SIO_SUBMOD_HTTP,
    SIO_SUBMOD_RTSP,
    SIO_SUBMOD_BUTT
};

#define SIO_SUBMOD_EXPORTSYMS g_custom_submod
#define SIO_SUBMOD_EXPORTSYMS_INIT(submod) extern const struct sio_submod* SIO_SUBMOD_EXPORTSYMS = &submod

struct sio_submod
{
    int (*mod_name)(const char **name);
    int (*mod_version)(const char **version);
    int (*mod_type)(void);

    int (*install)(void);
    int (*getlocat)(const char **locations, int size);
    int (*mod_hook)(const char *modname, struct sio_submod *mod);
    int (*newconn)(struct sio_server *server);
    int (*unstall)(void);
};

#endif