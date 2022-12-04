#ifndef SIO_THREAD_PTHREAD_H_
#define SIO_THREAD_PTHREAD_H_

struct sio_thread_pri;

#ifdef __cplusplus
extern "C" {
#endif

unsigned long int sio_thread_pthread_create(struct sio_thread_pri *pri);

#ifdef __cplusplus
}
#endif

#endif