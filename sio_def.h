#ifndef SIO_DEF_H_
#define SIO_DEF_H_

#ifdef WIN32
#include <basetsd.h>
#else
#include <stdint.h>
#endif

#define SIO_SOCK_RECV_BUFFSIZE 4096

#ifdef WIN32
typedef UINT_PTR sio_uptr_t;

typedef UINT_PTR sio_fd_t;
typedef sio_fd_t sio_socket_t;

#else
typedef uintptr_t sio_uptr_t;

typedef int sio_fd_t;
typedef sio_fd_t sio_socket_t;
#endif

typedef struct
{
    const char *data;
    int length;
}sio_str_t;

#endif