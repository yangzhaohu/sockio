#ifndef SIO_RWLOCK_H_
#define SIO_RWLOCK_H_

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#ifdef WIN32
#define sio_rwlock SRWLOCK
#define SIO_RWLOCK_INITIALIZER SRWLOCK_INIT
#define sio_rwlock_init(rwlock) InitializeSRWLock(&rwlock)
#define sio_rwlock_rdlock(rwlock) AcquireSRWLockShared(&rwlock)
#define sio_rwlock_wrlock(rwlock) AcquireSRWLockExclusive(&rwlock)
#define sio_rwlock_unrdlock(rwlock) ReleaseSRWLockShared(&rwlock)
#define sio_rwlock_unwrlock(rwlock) ReleaseSRWLockExclusive(&rwlock)
#define sio_rwlock_destory(rwlock)
#else
#define sio_rwlock pthread_rwlock_t
#define SIO_RWLOCK_INITIALIZER PTHREAD_RWLOCK_INITIALIZER
#define sio_rwlock_init(rwlock) pthread_rwlock_init(&rwlock, NULL)
#define sio_rwlock_rdlock(rwlock) pthread_rwlock_rdlock(&rwlock)
#define sio_rwlock_wrlock(rwlock) pthread_rwlock_wrlock(&rwlock)
#define sio_rwlock_unrdlock(rwlock) pthread_rwlock_unlock(&rwlock)
#define sio_rwlock_unwrlock(rwlock) pthread_rwlock_unlock(&rwlock)
#define sio_rwlock_destory(rwlock) pthread_rwlock_destroy(&rwlock)
#endif

#endif