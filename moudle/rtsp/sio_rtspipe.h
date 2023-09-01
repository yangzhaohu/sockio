#ifndef SIO_RTPCHN_H_
#define SIO_RTPCHN_H_

enum sio_rtspipe_over
{
    SIO_RTSPIPE_OVERTCP,
    SIO_RTSPIPE_OVERUDP
};

enum sio_rtspipe_t
{
    SIO_RTSPIPE_VIDEO,
    SIO_RTSPIPE_AUDIO
};

enum sio_rtspchn_t
{
    SIO_RTSPCHN_RTP,
    SIO_RTSPCHN_RTCP
};

struct sio_rtspipe;
struct sio_socket;

struct sio_rtspipe_ops
{
    void (*rtpack)(struct sio_rtspipe *rtpipe, const char *data, int len);
};

enum sio_rtspipe_optcmd
{
    SIO_RTPCHN_PRIVATE,
    SIO_RTPCHN_OPS
};

union sio_rtspipe_opt
{
    void *private;
    struct sio_rtspipe_ops ops;
};

#ifdef __cplusplus
extern "C" {
#endif

struct sio_rtspipe *sio_rtspipe_create(struct sio_socket *rtsp, enum sio_rtspipe_over over);

int sio_rtspipe_open_videochn(struct sio_rtspipe *rtpipe, int rtp, int rtcp);

int sio_rtspipe_open_audiochn(struct sio_rtspipe *rtpipe, int rtp, int rtcp);

int sio_rtspipe_setopt(struct sio_rtspipe *rtpipe,
    enum sio_rtspipe_optcmd cmd, union sio_rtspipe_opt *opt);

int sio_rtspipe_getopt(struct sio_rtspipe *rtpipe,
    enum sio_rtspipe_optcmd cmd, union sio_rtspipe_opt *opt);

int sio_rtspipe_getchn(struct sio_rtspipe *rtpipe, enum sio_rtspipe_t pipe, enum sio_rtspchn_t chn);

int sio_rtspipe_peerchn(struct sio_rtspipe *rtpipe, enum sio_rtspipe_t pipe, enum sio_rtspchn_t chn);

int sio_rtspipe_rtpsend(struct sio_rtspipe *rtpipe, enum sio_rtspipe_t pipe, enum sio_rtspchn_t chn,
    const char *data, unsigned int len);

int sio_rtspipe_close(struct sio_rtspipe *rtpipe);

#ifdef __cplusplus
}
#endif

#endif