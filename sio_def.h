#ifndef SIO_DEF_H_
#define SIO_DEF_H_

#define SIO_SOCK_RECV_BUFFSIZE 4096

#ifdef _WIN32
typedef SOCKET sio_socket_t;
#else
typedef int sio_socket_t;
#endif

typedef struct
{
    const char *data;
    int length;
}sio_str_t;

#endif