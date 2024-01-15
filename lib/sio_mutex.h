#ifndef SIO_MUTEX_H_
#define SIO_MUTEX_H_

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#ifdef WIN32
#define sio_mutex CRITICAL_SECTION
#define SIO_MUTEX_INITIALIZER { 0 }
#define sio_mutex_init(mutex) InitializeCriticalSection(mutex)
#define sio_mutex_lock(mutex) EnterCriticalSection(mutex)
#define sio_mutex_trylock(mutex) -1
#define sio_mutex_unlock(mutex) LeaveCriticalSection(mutex)
#define sio_mutex_destory(mutex) DeleteCriticalSection(mutex)
#else
#define sio_mutex pthread_mutex_t
#define SIO_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#define sio_mutex_init(mutex) pthread_mutex_init(mutex, NULL)
#define sio_mutex_lock(mutex) pthread_mutex_lock(mutex)
#define sio_mutex_trylock(mutex) pthread_mutex_trylock(mutex)
#define sio_mutex_unlock(mutex) pthread_mutex_unlock(mutex)
#define sio_mutex_destory(mutex) pthread_mutex_destroy(mutex)
#endif

#endif