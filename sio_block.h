#ifndef SIO_BLOCK_ELEMENT_H_
#define SIO_BLOCK_ELEMENT_H_

struct sio_block;

struct sio_block_entry
{
    void *ptr;
};

#ifdef __cplusplus
extern "C" {
#endif

struct sio_block *sio_block_create(int blocks, int esize);

int sio_block_get(struct sio_block *block, struct sio_block_entry *entry);

int sio_block_lose(struct sio_block *block, struct sio_block_entry *entry);

int sio_block_destory(struct sio_block *block);

#ifdef __cplusplus
}
#endif

#endif