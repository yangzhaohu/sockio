#include "sio_block.h"
#include <stdlib.h>
#include <string.h>
#include "sio_common.h"
#include "sio_fifo.h"

#define SIO_ELEMENT_FIFO_DEFAULT_SIZE 128

struct sio_element_padding
{
    struct sio_fifo *ref;
};

struct sio_element
{
    void *mem;
    struct sio_fifo *efifo;
};

struct sio_block
{
    int esize;
    struct sio_element *cur;
    struct sio_fifo *free;
    struct sio_fifo *busy;
};

static inline
struct sio_element *sio_element_create(int esize)
{
    struct sio_element *ele = malloc(sizeof(struct sio_element));
    SIO_COND_CHECK_RETURN_VAL(!ele, NULL);

    ele->mem = malloc(SIO_ELEMENT_FIFO_DEFAULT_SIZE * esize);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!ele->mem, NULL,
        free(ele));

    memset(ele->mem, 0, SIO_ELEMENT_FIFO_DEFAULT_SIZE * esize);

    ele->efifo = sio_fifo_create(SIO_ELEMENT_FIFO_DEFAULT_SIZE, sizeof(void *));
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!ele->efifo, NULL,
        free(ele->mem),
        free(ele));

    char *ptr = ele->mem;
    for (int i = 0; i < SIO_ELEMENT_FIFO_DEFAULT_SIZE; i++) {
        struct sio_element_padding *epadding = (struct sio_element_padding *)ptr;
        epadding->ref = ele->efifo;
        sio_fifo_in(ele->efifo, &ptr, 1);
        ptr += esize;
    }

    return ele;
}

static inline
void sio_element_destory(struct sio_element *ele)
{
    free(ele->mem);
    sio_fifo_destory(ele->efifo);
    free(ele);
}

struct sio_block *sio_block_create(int blocks, int esize)
{
    struct sio_block *block = malloc(sizeof(struct sio_block));
    SIO_COND_CHECK_RETURN_VAL(!block, NULL);

    block->free = sio_fifo_create(blocks, sizeof(struct sio_element *));
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!block->free, NULL,
        free(block));
    
    block->busy = sio_fifo_create(blocks, sizeof(struct sio_element *));
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!block->busy, NULL,
        free(block->free),
        free(block));

    esize += sizeof(struct sio_element_padding);
    block->cur = sio_element_create(esize);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!block->busy, NULL,
        free(block->free),
        free(block->busy),
        free(block));

    block->esize = esize;

    return block;
}

int sio_block_get(struct sio_block *block, struct sio_block_entry *entry)
{
    SIO_COND_CHECK_RETURN_VAL(!block || !entry, -1);

    char *eleblk = NULL;
    int ret = sio_fifo_out(block->cur->efifo, &eleblk, 1);
    if (ret == 0) {
        struct sio_element *element = block->cur;
        if (sio_fifo_in(block->busy, &element, 1) != 0) {
            block->cur = NULL;
        }

        SIO_COND_CHECK_RETURN_VAL(block->cur, -1);

        if (sio_fifo_out(block->free, &block->cur, 1) == 0) {
            block->cur = sio_element_create(block->esize);
        }

        SIO_COND_CHECK_RETURN_VAL(!block->cur, -1);

        ret = sio_fifo_out(block->cur->efifo, &eleblk, 1);
        SIO_COND_CHECK_RETURN_VAL(!ret, -1);
    }

    entry->ptr = eleblk + sizeof(struct sio_element_padding);

    return 0;
}

int sio_block_lose(struct sio_block *block, struct sio_block_entry *entry)
{
    SIO_COND_CHECK_RETURN_VAL(!block || !entry, -1);

    struct sio_element_padding *epadding = entry->ptr - sizeof(struct sio_element_padding);
    SIO_COND_CHECK_RETURN_VAL((long)epadding <= 0, -1);

    int ret = sio_fifo_in(epadding->ref, &epadding, 1);
    SIO_COND_CHECK_RETURN_VAL(!ret, -1);

    return 0;
}

static inline
void sio_block_foreach_del(struct sio_fifo *fifo)
{
    for (int i = 0; i < SIO_ELEMENT_FIFO_DEFAULT_SIZE; i++) {
        struct sio_element *ele = NULL;
        int ret = sio_fifo_out(fifo, &ele, 1);
        if (ret != 0) {
            sio_element_destory(ele);
        }
    }
}

int sio_block_destory(struct sio_block *block)
{
    SIO_COND_CHECK_RETURN_VAL(!block, -1);

    sio_element_destory(block->cur);

    sio_block_foreach_del(block->free);
    sio_fifo_destory(block->free);

    sio_block_foreach_del(block->busy);
    sio_fifo_destory(block->busy);

    free(block);

    return 0;
}