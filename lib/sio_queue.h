#ifndef SIO_QUEUE_H_
#define SIO_QUEUE_H_

#include "sio_list.h"

struct sio_queue;

#ifdef __cplusplus
extern "C" {
#endif

struct sio_queue *sio_queue_create();

int sio_queue_push(struct sio_queue *queue, struct sio_list_head *entry);

struct sio_list_head *sio_queue_pop(struct sio_queue *queue);

int sio_queue_destory(struct sio_queue *queue);

#ifdef __cplusplus
}
#endif

#endif