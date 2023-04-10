#ifndef SIO_SERVFLOW_H_
#define SIO_SERVFLOW_H_

typedef unsigned char sio_thread_capacity;

struct sio_servflow;
struct sio_sockflow;

enum sio_servflow_proto
{
    SIO_SERVFLOW_TCP
};

struct sio_servflow_addr
{
    char addr[32];
    int port;
};

enum sio_servflow_optcmd
{
    SIO_SERVFLOW_OPS
};

struct sio_servflow_ops
{
    int (*flow_new)(struct sio_sockflow *flow);
    int (*flow_data)(struct sio_sockflow *flow, const char *data, int len);
    int (*flow_close)(struct sio_sockflow *flow);
};

union sio_servflow_opt
{
    struct sio_servflow_ops ops;
};

#ifdef __cplusplus
extern "C" {
#endif

struct sio_servflow *sio_servflow_create(enum sio_servflow_proto type);

struct sio_servflow *sio_servflow_create2(enum sio_servflow_proto type, sio_thread_capacity io, sio_thread_capacity flow);

int sio_servflow_setopt(struct sio_servflow *flow, enum sio_servflow_optcmd cmd, union sio_servflow_opt *opt);

int sio_servflow_listen(struct sio_servflow *flow, struct sio_servflow_addr *addr);

int sio_sockflow_write(struct sio_sockflow *flow, char *buf, int len);

int sio_sockflow_close(struct sio_sockflow *flow);

int sio_servflow_destory(struct sio_servflow *flow);

#ifdef __cplusplus
}
#endif

#endif