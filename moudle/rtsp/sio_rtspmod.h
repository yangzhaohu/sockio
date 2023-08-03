#ifndef SIO_RTSPMOD_H_
#define SIO_RTSPMOD_H_

#include "sio_submod.h"

#ifdef __cplusplus
extern "C" {
#endif

int sio_rtspmod_name(const char **name);
int sio_rtspmod_type();
int sio_rtspmod_version(const char **version);

int sio_rtspmod_create(void);

int sio_rtspmod_streamconn(sio_conn_t conn);

int sio_rtspmod_streamin(sio_conn_t conn, const char *data, int len);

int sio_rtspmod_streamclose(sio_conn_t conn);

int sio_rtspmod_destory(void);

#ifdef __cplusplus
}
#endif

#endif