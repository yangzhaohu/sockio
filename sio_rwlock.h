#ifndef SIO_RWLOCK_H_
#define SIO_RWLOCK_H_

#ifdef LINUX
#include <pthread.h>
#elif defined(WIN32)
#include <windows.h>
#endif

#ifdef LINUX
#define sio_rwlock pthread_mutex_t
#define SIO_RWLOCK_INITIALIZER PTHREAD_RWLOCK_INITIALIZER
#define sio_rwlock_init(mutex) pthread_mutex_init(&mutex, NULL)
#define sio_rwlock_lock(mutex) pthread_mutex_lock(&mutex)
#define sio_rwlock_trylock(mutex) pthread_mutex_trylock(&mutex)
#define sio_rwlock_unlock(mutex) pthread_mutex_unlock(&mutex)
#define sio_rwlock_destory(mutex) pthread_mutex_destroy(&mutex)

#elif defined(WIN32)
#define sio_rwlock CRITICAL_SECTION
#define SIO_RWLOCK_INITIALIZER { 0 }
#define sio_rwlock_init(mutex) InitializeCriticalSection(&mutex)
#define sio_rwlock_lock(mutex) EnterCriticalSection(&mutex)
#define sio_rwlock_trylock(mutex) -1
#define sio_rwlock_unlock(mutex) LeaveCriticalSection(&mutex)
#define sio_rwlock_destory(mutex) DeleteCriticalSection(&mutex)
#endif

#endif