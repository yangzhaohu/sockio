#ifndef SIO_SPINLOCK_H_
#define SIO_SPINLOCK_H_

#ifdef LINUX
#include <pthread.h>
#elif defined(WIN32)
#include <windows.h>
#endif

#ifdef LINUX
#define sio_spinlock pthread_spinlock_t
// #define SIO_SPINLOCK_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#define sio_spinlock_init(spin) pthread_spin_init(&spin, NULL)
#define sio_spinlock_lock(spin) pthread_spin_lock(&spin)
#define sio_spinlock_trylock(spin) pthread_spin_trylock(&spin)
#define sio_spinlock_unlock(spin) pthread_spin_unlock(&spin)
#define sio_spinlock_destory(spin) pthread_spin_destroy(&spin)

#elif defined(WIN32)
#define sio_spinlock int
// #define SIO_SPINLOCK_INITIALIZER 
#define sio_spinlock_init(spin) 
#define sio_spinlock_lock(spin) 
#define sio_spinlock_trylock(spin) 
#define sio_spinlock_unlock(spin) 
#define sio_spinlock_destory(spin) 

#endif

#endif