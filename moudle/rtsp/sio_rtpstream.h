#ifndef SIO_RTPSTREAM_H_
#define SIO_RTPSTREAM_H_

#include "rtpack/sio_rtpack.h"

struct sio_rtpstream;

struct sio_rtpstream_ops
{
    void (*rtpack)(struct sio_rtpstream *stream, struct sio_packet *packet);
};

enum sio_rtpstream_optcmd
{
    SIO_RTPSTREAM_PRIVATE,
    SIO_RTPSTREAM_OPS
};

union sio_rtpstream_opt
{
    void *private;
    struct sio_rtpstream_ops ops;
};

#ifdef __cplusplus
extern "C" {
#endif

struct sio_rtpstream *sio_rtpstream_open(const char* name);

int sio_rtpstream_setopt(struct sio_rtpstream *stream, 
    enum sio_rtpstream_optcmd cmd, union sio_rtpstream_opt *opt);

int sio_rtpstream_getopt(struct sio_rtpstream *stream, 
    enum sio_rtpstream_optcmd cmd, union sio_rtpstream_opt *opt);

int sio_rtpstream_close(struct sio_rtpstream *stream);

#ifdef __cplusplus
}
#endif

#endif