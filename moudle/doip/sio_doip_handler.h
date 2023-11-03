#ifndef SIO_DOIP_HANDLER_H
#define SIO_DOIP_HANDLER_H

struct sio_socket;
struct sio_doip_contbl;
struct doip_hdr;

struct sio_doip_msg
{
    char *data;
    unsigned int len;
};

#ifdef __cplusplus
extern "C" {
#endif

int sio_doip_handler(struct sio_socket *sock, struct sio_doip_contbl *contbl, struct sio_doip_msg *msg);

#ifdef __cplusplus
}
#endif

#endif