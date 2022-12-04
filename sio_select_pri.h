#ifndef SIO_SELECT_PRI_H_
#define SIO_SELECT_PRI_H_

#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/select.h>
#endif

#ifdef WIN32
#define SIO_FD_SETSIZE 1024
#else
#define SIO_FD_SETSIZE FD_SETSIZE
#endif

#ifdef WIN32
typedef struct sio_fd_set {
    u_int fd_count;               /* how many are SET? */
    SOCKET  fd_array[SIO_FD_SETSIZE];   /* an array of SOCKETs */
} sio_fd_set;

#define SIO_FD_SET(fd, set) do { \
    u_int __i; \
    for (__i = 0; __i < ((fd_set FAR *)(set))->fd_count; __i++) { \
        if (((fd_set FAR *)(set))->fd_array[__i] == (fd)) { \
            break; \
        } \
    } \
    if (__i == ((fd_set FAR *)(set))->fd_count) { \
        if (((fd_set FAR *)(set))->fd_count < SIO_FD_SETSIZE) { \
            ((fd_set FAR *)(set))->fd_array[__i] = (fd); \
            ((fd_set FAR *)(set))->fd_count++; \
        } \
    } \
} while(0, 0)

#else

typedef fd_set sio_fd_set;
#define SIO_FD_SET      FD_SET

#endif

#define SIO_FD_ZERO     FD_ZERO
#define SIO_FD_CLR      FD_CLR
#define SIO_FD_ISSET    FD_ISSET

struct sio_select_rwfds
{
    sio_fd_set rfds;
    sio_fd_set wfds;
};

#endif