#ifndef SIO_ATOMIC_H_
#define SIO_ATOMIC_H_

#ifdef __cplusplus
extern "C" {
#endif

// *ptr == oldval ? *ptr = newval , and return old val
int sio_val_compare_and_swap(int *ptr, int oldval, int newval);

// add and fetch old val
int sio_int_fetch_and_add(int *ptr, int val);
unsigned int sio_uint_fetch_and_add(unsigned int *ptr, unsigned int val);
long sio_long_fetch_and_add(long *ptr, long val);
unsigned long sio_ulong_fetch_and_add(unsigned long *ptr, unsigned long val);

// sub and fetch old val
int sio_int_fetch_and_sub(int *ptr, int val);
unsigned int sio_uint_fetch_and_sub(unsigned int *ptr, unsigned int val);
long sio_long_fetch_and_sub(long *ptr, long val);
unsigned long sio_ulong_fetch_and_sub(unsigned long *ptr, unsigned long val);


// add and fetch NEW val
int sio_int_add_and_fetch(int *ptr, int val);
unsigned int sio_uint_add_and_fetch(unsigned int *ptr, unsigned int val);
long sio_long_add_and_fetch(long *ptr, long val);
unsigned long sio_ulong_add_and_fetch(unsigned long *ptr, unsigned long val);

// sub and fetch NEW val
int sio_int_sub_and_fetch(int *ptr, int val);
unsigned int sio_uint_sub_and_fetch(unsigned int *ptr, unsigned int val);
long sio_long_sub_and_fetch(long *ptr, long val);
unsigned long sio_ulong_sub_and_fetch(unsigned long *ptr, unsigned long val);

#ifdef __cplusplus
}
#endif

#endif