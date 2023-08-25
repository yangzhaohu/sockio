#ifndef SIO_RTPOOL_H_
#define SIO_RTPOOL_H_

struct sio_rtpool;

#ifdef __cplusplus
extern "C" {
#endif

struct sio_rtpool *sio_rtpool_create();

int sio_rtpool_add(struct sio_rtpool *rtpool, const char *name, void *stream);

int sio_rtpool_find(struct sio_rtpool *rtpool, const char *name, void **stream);

int sio_rtpool_del(struct sio_rtpool *rtpool, const char *name);

int sio_rtpool_destory(struct sio_rtpool *rtpool);

#ifdef __cplusplus
}
#endif

#endif