#include "sio_aio.h"
#include <stdlib.h>
#include <string.h>
#include "sio_common.h"
#include "sio_event.h"
#include "sio_overlap.h"
#include "sio_aioseq.h"
#include "sio_log.h"

#define SIO_AIO_BUFF_PADDING 512

union sio_aioctx
{
#ifdef WIN32
    struct sio_overlap ctx;
#else
    struct sio_aioseq ctx;
#endif
};

static inline
void *sio_aiobuf_baseptr(sio_aiobuf buf)
{
    return (char *)buf - sizeof(union sio_aioctx);
}

// #ifdef WIN32

// static inline
// struct sio_overlap *sio_overlap_base_event(struct sio_event *event)
// {
//     char *ptr = sio_aiobuf_baseptr(event->buf.ptr);
//     SIO_COND_CHECK_RETURN_VAL(ptr == NULL, NULL);

//     memset(ptr, 0, sizeof(struct sio_overlap));

//     struct sio_overlap *ovlp = (struct sio_overlap *)ptr;
//     ovlp->wsabuf.buf = event->buf.ptr;
//     ovlp->wsabuf.len = event->buf.len;
//     ovlp->ptr = event->pri;
//     ovlp->events = event->events;
    
//     return ovlp;
// }

// #else

// static inline
// struct sio_aioseq *sio_aioseq_base_event(struct sio_event *event)
// {
//     char *ptr = sio_aiobuf_baseptr(event->buf.ptr);
//     SIO_COND_CHECK_RETURN_VAL(ptr == NULL, NULL);

//     memset(ptr, 0, sizeof(struct sio_aioseq));

//     struct sio_aioseq *seq = (struct sio_aioseq *)ptr;
//     seq->buf = event->buf.ptr;
//     seq->len = event->buf.len;
//     seq->ptr = event->pri;
//     seq->events = event->events;
    
//     return seq;
// }

// #endif

static inline
int sio_aioctx_size_imp()
{
    return sizeof(union sio_aioctx);
}

sio_aiobuf sio_aiobuf_alloc(unsigned long size)
{
    size += SIO_AIO_BUFF_PADDING; // for ssl

    int must = sio_aioctx_size_imp();
    char *buf = malloc(size + must);

    return buf + must;
}

void *sio_aiobuf_aioctx_ptr(sio_aiobuf buf)
{
    return sio_aiobuf_baseptr(buf);
}

void sio_aiobuf_free(sio_aiobuf buf)
{
    buf = sio_aiobuf_baseptr(buf);
    free(buf);
}

int sio_aioctx_size()
{
    return sio_aioctx_size_imp();
}

// int sio_aio_accept(sio_fd_t sfd, sio_fd_t fd, struct sio_event *event)
// {
// #ifdef WIN32
//     struct sio_overlap *ovlp = sio_overlap_base_event(event);
//     SIO_COND_CHECK_RETURN_VAL(ovlp == NULL, -1);

//     unsigned long recvsize = 0;
//     static char addrbuf[2 * (sizeof(struct sockaddr_in) + 16)] = { 0 };
//     int ret = AcceptEx(sfd, fd, addrbuf, 0, 
//         sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16, &recvsize, (LPOVERLAPPED)ovlp);

//     SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == 0 && WSAGetLastError() != ERROR_IO_PENDING, -1,
//         SIO_LOGE("iocp post accept failed\n"));

// #else
//     return -1;

// #endif

//     return 0;
// }

// int sio_aio_recv(sio_fd_t fd, struct sio_event *event)
// {
// #ifdef WIN32
//     struct sio_overlap *ovlp = sio_overlap_base_event(event);
//     SIO_COND_CHECK_RETURN_VAL(ovlp == NULL, -1);

//     int ret = WSARecv(fd, &ovlp->wsabuf, 1, NULL, &ovlp->olflags, &ovlp->overlap, NULL);

//     SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1 && WSAGetLastError() != ERROR_IO_PENDING, -1,
//         SIO_LOGE("iocp post recv failed\n"));

// #else
//     return -1;

// #endif
//     return 0;
// }

// int sio_aio_send(sio_fd_t fd, struct sio_event *event)
// {
// #ifdef WIN32
//     struct sio_overlap *ovlp = sio_overlap_base_event(event);
//     SIO_COND_CHECK_RETURN_VAL(ovlp == NULL, -1);

//     int ret = WSASend(fd, &ovlp->wsabuf, 1, NULL, ovlp->olflags, &ovlp->overlap, NULL);

//     SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1 && WSAGetLastError() != ERROR_IO_PENDING, -1,
//         SIO_LOGE("iocp post send failed, errno: %d\n", WSAGetLastError()));

// #else
//     return -1;

// #endif
//     return 0;
// }