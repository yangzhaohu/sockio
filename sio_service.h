#ifndef SIO_SERVICE_H_
#define SIO_SERVICE_H_

enum sio_service_type
{
    SIO_SERVICE_RAW = 1,
    SIO_SERVICE_HTTP,
    SIO_SERVICE_RTSP,
    SIO_SERVICE_DOIP
};

enum sio_service_optcmd
{
    SIO_SERVICE_ADDR
};

struct sio_service_addr
{
    char addr[32];
    int port;
};

union sio_service_opt
{
    struct sio_service_addr addr;
};

struct sio_location;
struct sio_submod;

#ifdef __cplusplus
extern "C" {
#endif

struct sio_service *sio_service_create(enum sio_service_type type);

int sio_service_setopt(struct sio_service *service, enum sio_service_optcmd cmd, union sio_service_opt *opt);

int sio_service_getopt(struct sio_service *service, enum sio_service_optcmd cmd, union sio_service_opt *opt);

int sio_service_setlocat(struct sio_service *service, const struct sio_location *locations, int size);

int sio_service_dowork(struct sio_service *service);

int sio_service_insmod(struct sio_service *service, const char *modname, struct sio_submod *mod);

int sio_service_destory(struct sio_service *service);

#ifdef __cplusplus
}
#endif

#endif