#ifndef SIO_DOIP_DISCOVER_H
#define SIO_DOIP_DISCOVER_H

struct sio_sockaddr;

#ifdef __cplusplus
extern "C" {
#endif

int sio_doip_discover_init(struct sio_sockaddr *addr);

#ifdef __cplusplus
}
#endif

#endif