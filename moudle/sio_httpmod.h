#ifndef SIO_HTTPMOD_H_
#define SIO_HTTPMOD_H_

#include "sio_submod.h"

#ifdef __cplusplus
extern "C" {
#endif

int sio_httpmod_name(const char **name);
int sio_httpmod_type();
int sio_httpmod_version(const char **version);

int sio_httpmod_create(void);

int sio_httpmod_streamconn(sio_conn_t conn);

int sio_httpmod_streamin(sio_conn_t conn, const char *data, int len);

int sio_httpmod_streamclose(sio_conn_t conn);

int sio_httpmod_hookmod(const char *modname, struct sio_submod *mod);

int sio_httpmod_destory(void);

#ifdef __cplusplus
}
#endif

#endif