#ifndef SIO_RTSPDEV_H_
#define SIO_RTSPDEV_H_

enum sio_rtsp_type
{
    SIO_RTSPTYPE_VOD,
    SIO_RTSPTYPE_LIVE,
    SIO_RTSPTYPE_LIVERECORD,
    SIO_RTSPTYPE_BUTT
};

typedef struct sio_rtspdev * sio_rtspdev_t;

struct sio_rtspipe;

struct sio_rtspdev
{
    sio_rtspdev_t dev;
    sio_rtspdev_t (*open)(const char *name);
    int (*setdescribe)(sio_rtspdev_t dev, const char *describe, int len);
    int (*describe)(sio_rtspdev_t dev, const char **describe);
    int (*play)(sio_rtspdev_t dev);
    int (*pause)(sio_rtspdev_t dev);
    int (*progress)(sio_rtspdev_t dev);
    int (*record)(sio_rtspdev_t dev, const char *data, unsigned int len);
    int (*add_senddst)(sio_rtspdev_t dev, struct sio_rtspipe *dst);
    int (*rm_senddst)(sio_rtspdev_t dev, struct sio_rtspipe *dst);
    int (*close)(sio_rtspdev_t dev);
};

#ifdef __cplusplus
extern "C" {
#endif

static inline
sio_rtspdev_t sio_rtspdev_open_default(const char *name)
{
    static int p = 0;
    return (sio_rtspdev_t)&p;
}

static inline
int sio_rtspdev_setdescribe_default(sio_rtspdev_t dev, const char **describe)
{
    return 0;
}

static inline
int sio_rtspdev_describe_default(sio_rtspdev_t dev, const char **describe)
{
    return 0;
}

static inline
int sio_rtspdev_play_default(sio_rtspdev_t dev)
{
    return 0;
}

static inline
int sio_rtspdev_pause_default(sio_rtspdev_t dev)
{
    return 0;
}

static inline
int sio_rtspdev_progress_default(sio_rtspdev_t dev)
{
    return 0;
}

static inline
int sio_rtspdev_record_default(sio_rtspdev_t dev, const char *data, unsigned int len)
{
    return 0;
}

static inline
int sio_rtspdev_add_senddst_default(sio_rtspdev_t dev, struct sio_rtspipe *dst)
{
    return 0;
}

static inline
int sio_rtspdev_rm_senddst_default(sio_rtspdev_t dev, struct sio_rtspipe *dst)
{
    return 0;
}

static inline
int sio_rtspdev_close_default(sio_rtspdev_t dev)
{
    return 0;
}

#ifdef __cplusplus
}
#endif

#endif