#ifndef SIO_SOCKET_PRI_H_
#define SIO_SOCKET_PRI_H_

#ifdef WIN32
// #define WIN32_LEAN_AND_MEAN
// #include <windows.h>
#include <winsock2.h>
// #include <io.h>
// #include <getopt.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#endif

#ifdef WIN32
#define CLOSE(fd) closesocket(fd)
#else
#define CLOSE(fd) close(fd)
#endif

#ifdef WIN32
#define sio_sock_errno WSAGetLastError()
#else
#define sio_sock_errno errno
#endif

#endif