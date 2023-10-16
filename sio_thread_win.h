#ifndef SIO_THREAD_WIN_H_
#define SIO_THREAD_WIN_H_

struct sio_thread_pri;

#ifdef __cplusplus
extern "C" {
#endif

unsigned long int sio_thread_win_create(struct sio_thread_pri *pri);

int sio_thread_win_join(unsigned long int tid);

int sio_thread_win_destory(unsigned long int tid);

#ifdef __cplusplus
}
#endif

#endif