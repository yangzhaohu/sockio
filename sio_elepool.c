#include "sio_elepool.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sio_common.h"
#include "sio_queue.h"
#include "sio_mutex.h"

struct sio_ele_entry
{
    struct sio_list_head head;
};

struct sio_elepool
{
    unsigned int elesize;
    void *(*eleinit)(void *memptr);
    void (*eleuninit)(void *memptr);
    struct sio_queue *queue;
    sio_mutex mtx;
};

static inline
int sio_elepool_newele(struct sio_elepool *pool, int size)
{
    unsigned int elesize = pool->elesize;
    struct sio_queue *queue = pool->queue;
    for (int i = 0; i < size; i++) {
        void *ele = malloc(sizeof(struct sio_ele_entry) + elesize);
        SIO_COND_CHECK_BREAK(ele == NULL);
        memset(ele, 0, sizeof(struct sio_ele_entry) + elesize);
        void *memptr = ele + sizeof(struct sio_ele_entry);
        SIO_COND_CHECK_CALLOPS_BREAK(pool->eleinit(memptr) != memptr,
            free(ele));
        
        sio_queue_push(queue, ele);
    }

    return 0;
}

struct sio_elepool *sio_elepool_create(void *(*eleinit)(void *memptr), void (*eleuninit)(void *memptr), unsigned int elesize)
{
    struct sio_elepool *elepool = malloc(sizeof(struct sio_elepool));
    SIO_COND_CHECK_RETURN_VAL(!elepool, NULL);

    memset(elepool, 0, sizeof(struct sio_elepool));
    elepool->elesize = elesize;
    elepool->eleinit = eleinit;
    elepool->eleuninit = eleuninit;

    struct sio_queue *queue = sio_queue_create();
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(queue == NULL, NULL,
        free(elepool));

    elepool->queue = queue;

    sio_mutex_init(&elepool->mtx);

    sio_elepool_newele(elepool, 256);

    return elepool;
}

void *sio_elepool_get_imp(struct sio_elepool *pool)
{
    struct sio_list_head *eletry = NULL;
    eletry = sio_queue_pop(pool->queue);

    return eletry;
}

void *sio_elepool_get(struct sio_elepool *pool)
{
    struct sio_list_head *eletry = NULL;

    sio_mutex_lock(&pool->mtx);
    eletry = sio_elepool_get_imp(pool);
    if (eletry == NULL) {
        sio_elepool_newele(pool, 256);
        eletry = sio_queue_pop(pool->queue);
    }
    sio_mutex_unlock(&pool->mtx);

    char *memptr = (char *)eletry;
    if (memptr) {
        memptr += sizeof(struct sio_ele_entry);
    }

    return memptr;
}

int sio_elepool_lose(struct sio_elepool *pool, void *ele)
{
    sio_mutex_lock(&pool->mtx);
    void *eletry = (char *)ele - sizeof(struct sio_ele_entry);
    int ret = sio_queue_push(pool->queue, eletry);
    sio_mutex_unlock(&pool->mtx);

    return ret;
}

int sio_elepool_destroy(struct sio_elepool *pool)
{
    sio_mutex_lock(&pool->mtx);
    while (1) {
        struct sio_list_head *eletry = NULL;
        eletry = sio_elepool_get_imp(pool);
        char *memptr = (char *)eletry;
        if (memptr) {
            memptr += sizeof(struct sio_ele_entry);
            pool->eleuninit(memptr);
            free(eletry);
        } else {
            break;
        }
    }
    sio_mutex_unlock(&pool->mtx);

    sio_queue_destory(pool->queue);
    sio_mutex_destory(&pool->mtx);
    free(pool);

    return 0;
}