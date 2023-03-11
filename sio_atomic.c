#include "sio_atomic.h"
#ifdef WIN32
#include "windows.h"
#endif

#ifdef WIN32
#define SIO_FETCH_AND_ADD(ptr, val) InterlockedAdd((LONG *)ptr, val)
#define SIO_FETCH_AND_SUB(ptr, val) InterlockedAdd((LONG *)ptr, -val)
#define SIO_ADD_AND_FETCH(ptr, val) InterlockedAdd((LONG *)ptr, val) + val;
#define SIO_SUB_AND_FETCH(ptr, val) InterlockedAdd((LONG *)ptr, -val) - val;
#else
#define SIO_FETCH_AND_ADD(ptr, val)  __sync_fetch_and_add(ptr, val)
#define SIO_FETCH_AND_SUB(ptr, val)  __sync_fetch_and_sub(ptr, val)
#define SIO_ADD_AND_FETCH(ptr, val)  __sync_add_and_fetch(ptr, val)
#define SIO_SUB_AND_FETCH(ptr, val)  __sync_sub_and_fetch(ptr, val)
#endif

int sio_val_compare_and_swap(int *ptr, int oldval, int newval)
{
#ifdef WIN32
    return InterlockedCompareExchange((LONG *)ptr, newval, oldval);
#else
    return  __sync_val_compare_and_swap(ptr, oldval, newval);
#endif
}

// old val

int sio_int_fetch_and_add(int *ptr, int val)
{
    return SIO_FETCH_AND_ADD(ptr, val);
}

unsigned int sio_uint_fetch_and_add(unsigned int *ptr, unsigned int val)
{
    return SIO_FETCH_AND_ADD(ptr, val);
}

long sio_long_fetch_and_add(long *ptr, long val)
{
    return SIO_FETCH_AND_ADD(ptr, val);
}

unsigned long sio_ulong_fetch_and_add(unsigned long *ptr, unsigned long val)
{
    return SIO_FETCH_AND_ADD(ptr, val);
}

int sio_int_fetch_and_sub(int *ptr, int val)
{
    return SIO_FETCH_AND_SUB(ptr, val);
}

unsigned int sio_uint_fetch_and_sub(unsigned int *ptr, unsigned int val)
{
    return SIO_FETCH_AND_SUB(ptr, val);
}

long sio_long_fetch_and_sub(long *ptr, long val)
{
    return SIO_FETCH_AND_SUB(ptr, val);
}

unsigned long sio_ulong_fetch_and_sub(unsigned long *ptr, unsigned long val)
{
    return SIO_FETCH_AND_SUB(ptr, val);
}

// new val

int sio_int_add_and_fetch(int *ptr, int val)
{
    return SIO_ADD_AND_FETCH(ptr, val);
}

unsigned int sio_uint_add_and_fetch(unsigned int *ptr, unsigned int val)
{
    return SIO_ADD_AND_FETCH(ptr, val);
}

long sio_long_add_and_fetch(long *ptr, long val)
{
    return SIO_ADD_AND_FETCH(ptr, val);
}

unsigned long sio_ulong_add_and_fetch(unsigned long *ptr, unsigned long val)
{
    return SIO_ADD_AND_FETCH(ptr, val);
}

int sio_int_sub_and_fetch(int *ptr, int val)
{
    return SIO_SUB_AND_FETCH(ptr, val);
}

unsigned int sio_uint_sub_and_fetch(unsigned int *ptr, unsigned int val)
{
    return SIO_SUB_AND_FETCH(ptr, val);
}

long sio_long_sub_and_fetch(long *ptr, long val)
{
    return SIO_SUB_AND_FETCH(ptr, val);
}

unsigned long sio_ulong_sub_and_fetch(unsigned long *ptr, unsigned long val)
{
    return SIO_SUB_AND_FETCH(ptr, val);
}