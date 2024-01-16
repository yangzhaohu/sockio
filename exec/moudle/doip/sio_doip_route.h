#ifndef SIO_DOIP_ROUTE_H
#define SIO_DOIP_ROUTE_H

struct sio_doip_contbl;
struct doip_route;

#ifdef __cplusplus
extern "C" {
#endif

int sio_doip_route_process(struct sio_doip_contbl *contbl, struct doip_route *route);

#ifdef __cplusplus
}
#endif

#endif