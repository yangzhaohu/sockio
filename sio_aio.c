#include "sio_aio.h"
#include <string.h>
#include "sio_common.h"
#include "sio_event.h"
#include "sio_overlap.h"
#include "sio_log.h"

#ifdef WIN32

static inline
struct sio_overlap *sio_overlap_base_event(struct sio_event *event)
{
    char *ptr = event->buf.ptr;
    int len = event->buf.len;
    len -= sizeof(struct sio_overlap);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ptr == NULL || len <= 0, NULL,
        SIO_LOGE("enough buffer space for overlap access. minimal space %u",
            sizeof(struct sio_overlap)));
    ptr += len;
    memset(ptr, 0, sizeof(struct sio_overlap));

    struct sio_overlap *ovlp = (struct sio_overlap *)ptr;
    ovlp->wsabuf.buf = event->buf.ptr;
    ovlp->wsabuf.len = len;
    ovlp->ptr = event->owner.pri;
    ovlp->fd = event->owner.fd;
    ovlp->events = event->events;
    
    return ovlp;
}

#endif

int sio_aio_accept(sio_fd_t sfd, sio_fd_t fd, struct sio_event *event)
{
#ifdef WIN32
    struct sio_overlap *ovlp = sio_overlap_base_event(event);
    SIO_COND_CHECK_RETURN_VAL(ovlp == NULL, -1);

    unsigned long recvsize = 0;
    static char addrbuf[2 * (sizeof(struct sockaddr_in) + 16)] = { 0 };
    int ret = AcceptEx(sfd, fd, addrbuf, 0, 
        sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16, &recvsize, (LPOVERLAPPED)ovlp);

    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == 0 && WSAGetLastError() != ERROR_IO_PENDING, -1,
        SIO_LOGE("iocp post accept failed\n"));

#else
    return -1;

#endif

    return 0;
}

int sio_aio_recv(sio_fd_t fd, struct sio_event *event)
{
    #ifdef WIN32
    struct sio_overlap *ovlp = sio_overlap_base_event(event);
    SIO_COND_CHECK_RETURN_VAL(ovlp == NULL, -1);

    int ret = WSARecv(fd, &ovlp->wsabuf, 1, NULL, &ovlp->olflags, &ovlp->overlap, NULL);

    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1 && WSAGetLastError() != ERROR_IO_PENDING, -1,
        SIO_LOGE("iocp post recv failed\n"));

#else
    return -1;

#endif
    return 0;
}

int sio_aio_send(sio_fd_t fd, struct sio_event *event)
{
    return -1;
}