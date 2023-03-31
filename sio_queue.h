#ifndef SIO_QUEUE_H_
#define SIO_QUEUE_H_

struct sio_queue;

#ifdef __cplusplus
extern "C" {
#endif

struct sio_queue *sio_queue_create();

int sio_queue_destory(struct sio_queue *queue);

#ifdef __cplusplus
}
#endif

#endif