#ifndef SIO_SOCKSSL_H_
#define SIO_SOCKSSL_H_

#include "sio_def.h"

#define SIO_SOCKSSL_ESSL            -1
#define SIO_SOCKSSL_EWANTREAD       -2
#define SIO_SOCKSSL_EWANTWRITE      -3
#define SIO_SOCKSSL_EZERORETURN     -6

struct sio_sockssl;

enum sio_ssloptc
{
    /* set ca cert */
    SIO_SSL_CACERT,
    /* set user cert */
    SIO_SSL_USERCERT,
    /* set user private key */
    SIO_SSL_USERKEY,
};

union sio_sslopt
{
    const char *data;
};

#ifdef __cplusplus
extern "C" {
#endif

struct sio_sockssl *sio_sockssl_create();
struct sio_sockssl *sio_sockssl_create2(sio_fd_t fd);

int sio_sockssl_setopt(struct sio_sockssl *ssock, enum sio_ssloptc cmd, union sio_sslopt *opt);

int sio_sockssl_setfd(struct sio_sockssl *ssock, sio_fd_t fd);

int sio_sockssl_accept(struct sio_sockssl *ssock);

int sio_sockssl_connect(struct sio_sockssl *ssock);

int sio_sockssl_handshake(struct sio_sockssl *ssock);

int sio_sockssl_read(struct sio_sockssl *ssock, char *buf, int len);

int sio_sockssl_write(struct sio_sockssl *ssock, const char *data, int len);

int sio_sockssl_shutdown(struct sio_sockssl *ssock);

int sio_sockssl_destory(struct sio_sockssl *ssock);

#ifdef __cplusplus
}
#endif

#endif