#ifndef SIO_SUBMOD_H_
#define SIO_SUBMOD_H_

#include "moudle/sio_locate.h"

enum sio_submod_type
{
    SIO_SUBMOD_RAW = 1,
    SIO_SUBMOD_HTTP,
    SIO_SUBMOD_BUTT
};

struct sio_conn;
typedef struct sio_conn * sio_conn_t;

enum sio_conn_optcmd
{
    SIO_CONN_PRIVATE
};

union sio_conn_opt
{
    void *private;
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

#ifdef __cplusplus
extern "C" {
#endif

int sio_conn_setopt(sio_conn_t conn, enum sio_conn_optcmd cmd, union sio_conn_opt *opt);

int sio_conn_getopt(sio_conn_t conn, enum sio_conn_optcmd cmd, union sio_conn_opt *opt);

int sio_conn_write(sio_conn_t conn, const char *buf, int len);

int sio_conn_close(sio_conn_t conn);

#ifdef __cplusplus
}
#endif

#endif