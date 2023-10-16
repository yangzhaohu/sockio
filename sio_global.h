#ifndef SIO_GLOBAL_H_
#define SIO_GLOBAL_H_

#ifdef __cplusplus
extern "C" {
#endif

int sio_global_sigaction(int sig);

int sio_global_sigmask(int sig, int how);

int sio_global_sigwait(int sig);

int sio_global_sigwaitinfo(int sig, void *sigptr);

#ifdef __cplusplus
}
#endif

#endif