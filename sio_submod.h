#ifndef SIO_SUBMOD_H_
#define SIO_SUBMOD_H_

#include "moudle/sio_locate.h"
#include "moudle/sio_conn.h"

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
    int (*unstall)(void);

    int (*stream_conn)(sio_conn_t conn);
    int (*stream_close)(sio_conn_t conn);

    int (*stream_seg)(sio_conn_t conn, int type, const char *data, int len);
};

#endif