#ifndef SIO_TASKFIFO_H_
#define SIO_TASKFIFO_H_

struct sio_taskfifo;

#ifdef __cplusplus
extern "C" {
#endif

struct sio_taskfifo *sio_taskfifo_create(unsigned short pipe, void *(*routine)(void *handle, void *data),
                                    void *arg, unsigned int size);

int sio_taskfifo_in(struct sio_taskfifo *tf, unsigned short pipe, void *entry, unsigned int size);

int sio_taskfifo_out(struct sio_taskfifo *tf, unsigned short pipe, void *entry, unsigned int size);

int sio_taskfifo_destory(struct sio_taskfifo *tf);

#ifdef __cplusplus
}
#endif

#endif