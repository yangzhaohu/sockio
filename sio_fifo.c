#include "sio_fifo.h"
#include <stdlib.h>
#include <string.h>
#include "sio_common.h"

struct sio_fifo
{
	unsigned int in;
	unsigned int out;
    unsigned int mask;
	unsigned int esize;
	void *data;
};

static inline
unsigned int sio_fifo_free_size(struct sio_fifo *fifo)
{
    return (fifo->mask + 1) - (fifo->in - fifo->out);
}

static inline
unsigned int sio_fifo_pow_of_two(unsigned int size)
{
    unsigned int tmp = 1;
    while (1) {
        tmp *= 2;
        if (tmp >= size) {
            break;
        }
    }

    return tmp;
}

struct sio_fifo *sio_fifo_create(unsigned int size, unsigned int esize)
{
    struct sio_fifo *fifo = malloc(sizeof(struct sio_fifo));
    SIO_COND_CHECK_RETURN_VAL(fifo == NULL, NULL);

    memset(fifo, 0, sizeof(struct sio_fifo));

    size = sio_fifo_pow_of_two(size);

    fifo->data = malloc(size * esize);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(fifo->data == NULL, NULL,
        free(fifo));

    fifo->esize = esize;
    fifo->mask = size - 1;

    return fifo;
}

static inline
void sio_fifo_copy_in(struct sio_fifo *fifo, const sio_fifo_ele *buf,
    unsigned int len, unsigned int off)
{
    unsigned int size = fifo->mask + 1;
    unsigned int esize = fifo->esize;
    unsigned int l;

    off &= fifo->mask;
    if (esize != 1) {
        size *= esize;
        len *= esize;
        off *= esize;
    }
    l = len < (size - off) ? len : (size - off);

    memcpy(fifo->data + off, buf, l);
    memcpy(fifo->data, buf + len, len - l);

    // smp_wmb();
}

int sio_fifo_in(struct sio_fifo *fifo, const sio_fifo_ele *buf, unsigned int len)
{
    unsigned int l;

    l = sio_fifo_free_size(fifo);
    l = l < len ? l : len;

    sio_fifo_copy_in(fifo, buf, len, fifo->in);

    fifo->in += l;

    return l;
}

static inline
void sio_fifo_copy_out(struct sio_fifo *fifo, sio_fifo_ele *buf,
    unsigned int len, unsigned int off)
{
    unsigned int size = fifo->mask + 1;
    unsigned int esize = fifo->esize;
    unsigned int l;

    off &= fifo->mask;
    if (esize != 1) {
        size *= esize;
        len *= esize;
        off *= esize;
    }
    l = len < (size - off) ? len : (size - off);

    memcpy(buf, fifo->data + off, l);
    memcpy(buf + len, fifo->data, len - l);

    // smp_wmb();
}

int sio_fifo_out(struct sio_fifo *fifo, sio_fifo_ele *buf, unsigned int len)
{
    unsigned int l;

    l = fifo->in - fifo->out;
    l = l < len ? l : len;

    sio_fifo_copy_out(fifo, buf, len, fifo->out);

    fifo->out += l;

    return l;
}

int sio_fifo_destory(struct sio_fifo *fifo)
{
    free(fifo->data);
    free(fifo);

    return 0;
}