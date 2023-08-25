#ifndef SIO_RTPCHN_H_
#define SIO_RTPCHN_H_

enum sio_rtpchn_over
{
    SIO_RTPCHN_OVER_TCP,
    SIO_RTPCHN_OVER_UDP
};

struct sio_rtpchn;
struct sio_socket;

struct sio_rtpchn_ops
{
    void (*rtpack)(struct sio_rtpchn *rtpchn, const char *data, int len);
};

enum sio_rtpchn_optcmd
{
    SIO_RTPCHN_PRIVATE,
    SIO_RTPCHN_OPS
};

union sio_rtpchn_opt
{
    void *private;
    struct sio_rtpchn_ops ops;
};

#ifdef __cplusplus
extern "C" {
#endif

struct sio_rtpchn *sio_rtpchn_overtcp_open(struct sio_socket *rtsp, int rtp, int rtcp);

struct sio_rtpchn *sio_rtpchn_overudp_open(struct sio_socket *rtsp, int rtp, int rtcp);

int sio_rtpchn_setopt(struct sio_rtpchn *rtpchn,
    enum sio_rtpchn_optcmd cmd, union sio_rtpchn_opt *opt);

int sio_rtpchn_getopt(struct sio_rtpchn *rtpchn,
    enum sio_rtpchn_optcmd cmd, union sio_rtpchn_opt *opt);

int sio_rtpchn_chnrtp(struct sio_rtpchn *rtpchn);

int sio_rtpchn_chnrtcp(struct sio_rtpchn *rtpchn);

int sio_rtpchn_rtpsend(struct sio_rtpchn *rtpchn, const char *data, unsigned int len);

int sio_rtpchn_close(struct sio_rtpchn *rtpchn);

#ifdef __cplusplus
}
#endif

#endif