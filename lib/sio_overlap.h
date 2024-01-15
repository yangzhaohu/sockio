#ifndef SIO_OVERLAP_H_
#define SIO_OVERLAP_H_

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <mswsock.h>

struct sio_overlap
{
    OVERLAPPED overlap;
    WSABUF wsabuf;
    void *ptr;
    unsigned int events;
    unsigned long olflags;
};

#endif

#endif