#include "sio_queue.h"
#include <stdlib.h>
#include <string.h>
#include "sio_common.h"
#include "sio_list.h"

struct sio_queue
{
    struct sio_list_head head;
    int count;
};

struct sio_queue *sio_queue_create()
{
    struct sio_queue *queue = malloc(sizeof(struct sio_queue));
    SIO_COND_CHECK_RETURN_VAL(!queue, NULL);

    memset(queue, 0, sizeof(struct sio_queue));

    sio_list_init(&queue->head);

    return queue;
}

int sio_queue_push(struct sio_queue *queue, struct sio_list_head *entry)
{
    SIO_COND_CHECK_RETURN_VAL(!queue, -1);

    return -1;
}

struct sio_list_head *sio_queue_pop(struct sio_queue *queue)
{
    SIO_COND_CHECK_RETURN_VAL(!queue, NULL);

    return NULL;
}

int sio_queue_destory(struct sio_queue *queue)
{
    SIO_COND_CHECK_RETURN_VAL(!queue, -1);

    free(queue);

    return 0;
}