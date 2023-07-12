#ifndef SIO_SUBMOD_H_
#define SIO_SUBMOD_H_

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

struct sio_mod
{
    int (*mod_name)(const char **name);
    int (*mod_version)(const char **version);
    int (*mod_type)(void);

    int (*install)(void);
    int (*mod_hook)(const char *modname, struct sio_mod *mod);
    int (*unstall)(void);

    int (*stream_conn)(sio_conn_t conn);
    int (*stream_in)(sio_conn_t conn, const char *data, int len);
    int (*stream_close)(sio_conn_t conn);
};

struct sio_mod_ops
{
    int a;
};

#ifdef __cplusplus
extern "C" {
#endif

int sio_conn_setopt(sio_conn_t conn, enum sio_conn_optcmd cmd, union sio_conn_opt *opt);

int sio_conn_getopt(sio_conn_t conn, enum sio_conn_optcmd cmd, union sio_conn_opt *opt);

int sio_conn_write(sio_conn_t conn, char *buf, int len);

int sio_conn_close(sio_conn_t conn);

#ifdef __cplusplus
}
#endif

#endif