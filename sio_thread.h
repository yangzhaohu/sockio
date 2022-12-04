#ifndef SIO_THREAD_H_
#define SIO_THREAD_H_

//typedef void (*thread_routine)(void *arg);

struct sio_thread;

#ifdef __cplusplus
extern "C" {
#endif

struct sio_thread *sio_thread_create(void *(*routine)(void *arg), void *arg);

int sio_thread_start(struct sio_thread *thread);

// int sio_thread_stop(struct sio_thread *thread);

int sio_thread_destory(struct sio_thread *thread);


#ifdef __cplusplus
}
#endif

#endif