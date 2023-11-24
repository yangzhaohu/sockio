#ifndef SIO_SOCKSSL_H_
#define SIO_SOCKSSL_H_

#include "sio_def.h"

struct sio_sockssl;

#ifdef __cplusplus
extern "C" {
#endif

struct sio_sockssl *sio_sockssl_create();
struct sio_sockssl *sio_sockssl_create2(sio_fd_t fd);

int sio_sockssl_setfd(struct sio_sockssl *ssock, sio_fd_t fd);

int sio_sockssl_handshake(struct sio_sockssl *ssock);

int sio_sockssl_destory(struct sio_sockssl *ssock);

#ifdef __cplusplus
}
#endif

#endif