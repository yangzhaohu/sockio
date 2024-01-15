#ifndef SIO_COND_H_
#define SIO_COND_H_

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif
#include "sio_mutex.h"

#ifdef WIN32
#define sio_cond CONDITION_VARIABLE
#define sio_cond_init(cond) InitializeConditionVariable(cond)
#define sio_cond_wake(cond) WakeConditionVariable(cond)
#define sio_cond_allwake(cond) WakeAllConditionVariable(cond)
#define sio_cond_wait(cond, mutex) SleepConditionVariableCS(cond, mutex, -1)
#define sio_cond_timedwait(cond, mutex, timeout) SleepConditionVariableCS(cond, mutex, timeout)
#define sio_cond_destory(cond)
#else
#define sio_cond pthread_cond_t
#define sio_cond_init(cond) pthread_cond_init(cond, NULL)
#define sio_cond_wake(cond) pthread_cond_signal(cond)
#define sio_cond_allwake(cond) pthread_cond_broadcast(cond)
#define sio_cond_wait(cond, mutex) pthread_cond_wait(cond, mutex)
#define sio_cond_timedwait(cond, mutex, timeout) pthread_cond_timedwait(cond, mutex, timeout)
#define sio_cond_destory(cond) pthread_cond_destroy(cond)
#endif

#endif