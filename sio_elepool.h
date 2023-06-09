#ifndef SIO_ELEPOOL_H_
#define SIO_ELEPOOL_H_

struct sio_elepool;

#ifdef __cplusplus
extern "C" {
#endif

struct sio_elepool *sio_elepool_create(void *(*eleinit)(void *memptr), void (*eleuninit)(void *memptr), unsigned int elesize);

void *sio_elepool_get(struct sio_elepool *pool);

int sio_elepool_lose(struct sio_elepool *pool, void *ele);

int sio_elepool_destroy(struct sio_elepool *pool);

#ifdef __cplusplus
}
#endif

#endif