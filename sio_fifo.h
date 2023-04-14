#ifndef SIO_FIFO_H_
#define SIO_FIFO_H_

typedef void sio_fifo_ele;

struct sio_fifo;

#ifdef __cplusplus
extern "C" {
#endif

struct sio_fifo *sio_fifo_create(unsigned int size, unsigned int esize);

int sio_fifo_in(struct sio_fifo *fifo, const sio_fifo_ele *buf, unsigned int count);

int sio_fifo_out(struct sio_fifo *fifo, sio_fifo_ele *buf, unsigned int count);

int sio_fifo_destory(struct sio_fifo *fifo);

#ifdef __cplusplus
}
#endif

#endif