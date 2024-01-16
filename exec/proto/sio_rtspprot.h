#ifndef SIO_RTSPPROT_H_
#define SIO_RTSPPROT_H_

enum sio_rtsp_method
{
    SIO_RTSP_INVALID,
    SIO_RTSP_OPTIONS,
    SIO_RTSP_DESCRIBE,
    SIO_RTSP_SETUP,
    SIO_RTSP_PLAY,
    SIO_RTSP_PAUSE,
    SIO_RTSP_TEARDOWN,
    SIO_RTSP_ANNOUNCE,
    SIO_RTSP_GET_PARAMETER,
    SIO_RTSP_RECORD,
    SIO_RTSP_REDIRECT,
    SIO_RTSP_SET_PARAMETER
};

struct sio_rtspprot;

struct sio_rtspprot_ops
{
    int (*prot_begin)(struct sio_rtspprot *prot, const char *at);
    int (*prot_method)(struct sio_rtspprot *prot, const char *at, int len);
    int (*prot_url)(struct sio_rtspprot *prot, const char *at, int len);
    int (*prot_filed)(struct sio_rtspprot *prot, const char *at, int len);
    int (*prot_value)(struct sio_rtspprot *prot, const char *at, int len);
    int (*prot_complete)(struct sio_rtspprot *prot);
};

enum sio_rtspprot_optcmd
{
    SIO_RTSPPROT_PRIVATE,
    SIO_RTSPPROT_OPS
};

union sio_rtspprot_opt
{
    void *private;
    struct sio_rtspprot_ops ops;
};

#ifdef __cplusplus
extern "C" {
#endif

struct sio_rtspprot *sio_rtspprot_create(void);

int sio_rtspprot_setopt(struct sio_rtspprot *rtspprot, enum sio_rtspprot_optcmd cmd, union sio_rtspprot_opt *opt);

int sio_rtspprot_getopt(struct sio_rtspprot *rtspprot, enum sio_rtspprot_optcmd cmd, union sio_rtspprot_opt *opt);

int sio_rtspprot_process(struct sio_rtspprot *rtspprot, const char *data, unsigned int len);

int sio_rtspprot_method(struct sio_rtspprot *rtspprot);

int sio_rtspprot_destory(struct sio_rtspprot *rtspprot);

#ifdef __cplusplus
}
#endif

#endif