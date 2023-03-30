#ifndef SIO_MEMPOOL_H_
#define SIO_MEMPOOL_H_

struct sio_mempool;

#ifdef __cplusplus
extern "C" {
#endif

struct sio_mempool *sio_mempool_create(unsigned int size);

int sio_mempool_destory(struct sio_mempool *pool);

#ifdef __cplusplus
}
#endif

#endif
