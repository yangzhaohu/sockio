#ifndef SIO_RTPLIVE_H_
#define SIO_RTPLIVE_H_

#include "sio_rtpchn.h"

struct sio_rtplive;
struct sio_socket;

#ifdef __cplusplus
extern "C" {
#endif

struct sio_rtplive *sio_rtplive_create();

int sio_rtplive_open(struct sio_rtplive *rtplive, const char *name);

int sio_rtplive_start(struct sio_rtplive *rtplive);

int sio_rtplive_attach_rtpchn(struct sio_rtplive *rtplive, struct sio_rtpchn *rtpchn);

int sio_rtplive_record(struct sio_rtplive *rtplive, const char *data, unsigned int len);

struct sio_rtplive *sio_rtplive_destory(struct sio_rtplive *rtplive);

#ifdef __cplusplus
}
#endif

#endif