#include "sio_rtpstream.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sio_common.h"
#include "rtpack/sio_avdev.h"
#include "sio_thread.h"
#include "sio_log.h"

struct sio_rtpstream_owner
{
    void *private;
    struct sio_rtpstream_ops ops;
};

struct sio_rtpstream
{
    struct sio_rtpstream_owner owner;
    struct sio_avdev dev;
    struct sio_rtpack packet;
    struct sio_thread *thread;
    int loop;
};

static inline
void sio_rtpstream_rtpack(void *handle, struct sio_packet *packet)
{
    struct sio_rtpstream *stream = handle;
    struct sio_rtpstream_owner *owner = &stream->owner;

    if (owner->ops.rtpack) {
        owner->ops.rtpack(stream, packet);
    }
}

static inline
void *sio_rtpstream_proc_routine(void *arg)
{
    struct sio_rtpstream *stream = (struct sio_rtpstream *)arg;
    struct sio_avdev *dev = &stream->dev;
    struct sio_rtpack *packet = &stream->packet;

    while (stream->loop == 0) {
        struct sio_avframe frame = { 0 };
        int ret = dev->readframe(dev->handle, &frame);
        SIO_COND_CHECK_CALLOPS_CONTINUE(ret == -1, usleep(40 * 1000));

        packet->packet(packet->handle, &frame, stream, sio_rtpstream_rtpack);

        usleep(40 * 1000);
    }
    return NULL;
}

static inline
int sio_rtpstream_avdev_open(struct sio_rtpstream *stream, const char* name)
{
    struct sio_avdev *dev = sio_avdev_find(SIO_AVDEV_JPEG);
    SIO_COND_CHECK_RETURN_VAL(!dev, -1);
    
    memcpy(&stream->dev, dev, sizeof(struct sio_avdev));

    dev = &stream->dev;

    dev->handle = dev->open(name);
    SIO_COND_CHECK_RETURN_VAL(!dev->handle, -1);

    struct sio_rtpack *packet = sio_rtpack_find(SIO_RTPACK_JPEG);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!packet, -1,
        dev->close(dev->handle));
    
    memcpy(&stream->packet, packet, sizeof(struct sio_rtpack));
    
    packet = &stream->packet;
    packet->handle = packet->open(1420);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!packet->handle, -1,
        dev->close(dev->handle));

    return 0;
}

static inline
int sio_rtpstream_avdev_close(struct sio_rtpstream *stream)
{
    struct sio_avdev *dev = &stream->dev;
    int ret = dev->close(dev->handle);
    SIO_COND_CHECK_RETURN_VAL(ret != 0, -1);

    dev->handle = NULL;

    struct sio_rtpack *packet = &stream->packet;
    ret = packet->close(packet->handle);
    SIO_COND_CHECK_RETURN_VAL(ret != 0, -1);

    packet->handle = NULL;

    return 0;
}

static inline
struct sio_rtpstream *sio_rtpstream_create()
{
    struct sio_rtpstream *stream = malloc(sizeof(struct sio_rtpstream));
    SIO_COND_CHECK_RETURN_VAL(!stream, NULL);

    memset(stream, 0, sizeof(struct sio_rtpstream));

    struct sio_thread *thread = sio_thread_create(sio_rtpstream_proc_routine, stream);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!thread, NULL,
        free(stream));

    stream->thread = thread;

    return stream;
}

static inline
int sio_rtpstream_destory(struct sio_rtpstream *stream)
{
    free(stream);

    return 0;
}

struct sio_rtpstream *sio_rtpstream_open(const char* name)
{
    struct sio_rtpstream *stream = sio_rtpstream_create();
    SIO_COND_CHECK_RETURN_VAL(!stream, NULL);

    int ret = sio_rtpstream_avdev_open(stream, name);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, NULL);

    return stream;
}

int sio_rtpstream_start(struct sio_rtpstream *stream)
{
    SIO_COND_CHECK_RETURN_VAL(!stream, -1);

    struct sio_thread *thread = stream->thread;
    int ret = sio_thread_start(thread);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    return 0;
}

int sio_rtpstream_setopt(struct sio_rtpstream *stream, 
    enum sio_rtpstream_optcmd cmd, union sio_rtpstream_opt *opt)
{
    SIO_COND_CHECK_RETURN_VAL(!stream, -1);

    int ret = 0;
    struct sio_rtpstream_owner *owner = &stream->owner;
    switch (cmd) {
    case SIO_RTPSTREAM_PRIVATE:
        owner->private = opt->private;
        break;

    case SIO_RTPSTREAM_OPS:
        owner->ops = opt->ops;
        break;
    
    default:
        ret = -1;
        break;
    }

    return ret;
}

int sio_rtpstream_getopt(struct sio_rtpstream *stream, 
    enum sio_rtpstream_optcmd cmd, union sio_rtpstream_opt *opt)
{
    SIO_COND_CHECK_RETURN_VAL(!stream, -1);

    int ret = 0;
    struct sio_rtpstream_owner *owner = &stream->owner;
    switch (cmd) {
    case SIO_RTPSTREAM_PRIVATE:
        opt->private = owner->private;
        break;

    case SIO_RTPSTREAM_OPS:
        opt->ops = owner->ops;
        break;
    
    default:
        ret = -1;
        break;
    }

    return ret;
}

int sio_rtpstream_close(struct sio_rtpstream *stream)
{
    SIO_COND_CHECK_RETURN_VAL(!stream, -1);

    stream->loop = 1;
    sio_thread_destory(stream->thread);

    sio_rtpstream_avdev_close(stream);
    sio_rtpstream_destory(stream);

    return 0;
}