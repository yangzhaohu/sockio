#include "sio_socket.h"
#include <string.h>
#include <stdlib.h>
#ifdef LINUX
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#elif defined(WIN32)
// #define WIN32_LEAN_AND_MEAN
// #include <windows.h>
#include <winsock2.h>
#include <io.h>
// #include <getopt.h>
#endif
#include "sio_common.h"
#include "sio_def.h"
#include "sio_event.h"
#include "sio_mplex.h"
#include "sio_io.h"
#include "sio_log.h"

// #ifdef WIN32
// #pragma comment(lib, "ws2_32.lib")
// #endif
    
enum sio_socket_mean
{
    SIO_SOCK_MEAN_SOCKET,
    SIO_SOCK_MEAN_SERVER
};

struct sio_socket_attr
{
    enum sio_socket_proto proto;
    enum sio_socket_mean mean;
    int nonblock;
};

struct sio_socket_owner
{
    void *uptr;
    struct sio_io_ops ops;
};

struct sio_socket
{
    int fd;
    struct sio_socket_attr attr;
    struct sio_socket_owner owner;
    struct sio_mplex *mp;
    char extbuf[64];      // extension buffer, additional purposes, such as acceptex of iocp
};

#ifdef WIN32
#define CLOSE(fd) closesocket(fd)
#else
#define CLOSE(fd) close(fd)
#endif

// close fd and break
#define sio_socket_close_fd_break(sock, ops)                            \
    struct sio_event __event = { 0 };                                   \
    sio_mplex_ctl(sock->mp, SIO_EV_OPT_DEL, sock->fd, &__event);        \
    CLOSE(sock->fd);                                                    \
    sock->fd = -1;                                                      \
    if (ops) {                                                          \
        ops(sock, 0, 0);                                                \
    }                                                                   \
    break;                                                              \

// socket nonblock recv errno break
#ifdef LINUX
#define sio_socket_recv_errno_break()                   \
    if (errno == EAGAIN ||                              \
        errno == EWOULDBLOCK) {                         \
        break;                                          \
    }
#elif defined(WIN32)
#define sio_socket_recv_errno_break()                   \
    int __err = WSAGetLastError();                      \
    if ( __err == WSAEWOULDBLOCK) {                     \
        break;                                          \
    }
#endif


// server socket accept
#define sio_socket_server_accept(sock, ops)         \
    if (ops) {                                      \
        ops(sock, 0, 0);                            \
    }


// socket recv
#define sio_socket_socket_recv(sock, ops)                                       \
    char __buf[SIO_SOCK_RECV_BUFFSIZE] = { 0 };                                 \
    int len;                                                                    \
    do {                                                                        \
        len = recv(sock->fd, __buf, SIO_SOCK_RECV_BUFFSIZE, 0);                 \
        if (len < 0) {                                                          \
            sio_socket_recv_errno_break();                                      \
            sio_socket_close_fd_break(sock, ops);                               \
        }                                                                       \
        else if (len == 0) {                                                    \
            sio_socket_close_fd_break(sock, ops);                               \
        }                                                                       \
        if (ops) {                                                              \
            ops(sock, __buf, len);                                              \
        }                                                                       \
    } while (attr->nonblock);


// dispatch once
#define sio_socket_event_dispatch_once(event)                           \
    if (event->events & SIO_EVENTS_IN) {                                \
        if (attr->mean == SIO_SOCK_MEAN_SOCKET) {                       \
            sio_socket_socket_recv(sock, ops->read_cb);                 \
        } else {                                                        \
            sio_socket_server_accept(sock, ops->read_cb);               \
        }                                                               \
    }                                                                   \
    if (event->events & SIO_EVENTS_ASYNC_READ) {                        \
        if (ops->read_cb) {                                             \
            ops->read_cb(sock, event->buf.ptr, event->buf.len);         \
        }                                                               \
    }                                                                   \
    if (event->events & SIO_EVENTS_ASYNC_ACCEPT) {                      \
        if (ops->read_cb) {                                             \
            struct sio_socket *__sock =                                 \
            SIO_CONTAINER_OF(event->buf.ptr,                            \
                struct sio_socket,                                      \
                extbuf);                                                \
            ops->read_cb(sock, (const char *)__sock, 0);                \
        }                                                               \
    }                                                                   \
    if (event->events & SIO_EVENTS_OUT) {                               \
        if (ops->write_cb) {                                            \
            ops->write_cb(sock, 0, 0);                                  \
        }                                                               \
    }


extern int sio_socket_event_dispatch(struct sio_event *events, int count)
{
    for (int i = 0; i < count; i++) {
        struct sio_event *event = &events[i];
        struct sio_socket *sock = event->owner.ptr;
        if (sock == NULL) {
            continue;
        }
        struct sio_socket_attr *attr = &sock->attr;
        struct sio_socket_owner *owner = &sock->owner;
        struct sio_io_ops *ops = &owner->ops;
        // printf("socket fd: %d, event: %d\n", sock->fd, event->events);

        sio_socket_event_dispatch_once(event);
    }

    return 0;
}

static inline
int sio_socket_sock(enum sio_socket_proto proto)
{
    int fd;
    if (proto == SIO_SOCK_TCP) {
        fd = socket(PF_INET, SOCK_STREAM, 0);
    } else if (proto == SIO_SOCK_UDP) {
        fd = socket(PF_INET, SOCK_DGRAM, 0);
    } else {
        fd = -1;
    }

    return fd;
}

static inline 
unsigned short sio_socket_domain(enum sio_socket_proto proto)
{
    unsigned short domain = PF_INET;
    if (SIO_SOCK_TCP <= proto && SIO_SOCK_UDP <= proto) {
        domain = PF_INET;
    }

    return domain;
}

static inline
void sio_socket_set_private(struct sio_socket *sock, void *private)
{
    struct sio_socket_owner *owner = &sock->owner;
    owner->uptr = private;
}

static inline
void sio_socket_set_ops(struct sio_socket *sock, struct sio_io_ops ops)
{
    struct sio_socket_owner *owner = &sock->owner;
    owner->ops = ops;
}

static inline
int sio_socket_set_nonblock(struct sio_socket *sock, int nonblock)
{
#ifdef LINUX
    int opt = fcntl(sock->fd, F_GETFL);
    SIO_COND_CHECK_RETURN_VAL(opt == -1, -1);

    opt = nonblock ? opt | O_NONBLOCK : opt & ~O_NONBLOCK;
    int ret = fcntl(sock->fd, F_SETFL, opt);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

#elif defined(WIN32)
    unsigned long mode = 1;
    int ret = ioctlsocket(sock->fd, FIONBIO, &mode);
    SIO_COND_CHECK_RETURN_VAL(ret == SOCKET_ERROR, -1);

#endif

    sock->attr.nonblock = nonblock ? 1 : 0;
    return 0;
}

static inline
int sio_socket_set_addrreuse(struct sio_socket *sock, int reuse)
{
#ifdef LINUX
#elif defined(WIN32)
    char reused = reuse ? 1 : 0;
    int ret = setsockopt(sock->fd, SOL_SOCKET, SO_REUSEADDR, &reused, sizeof(char));
    SIO_COND_CHECK_RETURN_VAL(ret == SOCKET_ERROR, -1);

#endif
    return 0;
}

static inline 
void sio_socket_addr_in(struct sio_socket *sock, struct sio_socket_addr *addr, struct sockaddr_in *addr_in)
{
    unsigned short domain = sio_socket_domain(sock->attr.proto);
    
    addr_in->sin_family = domain;
    addr_in->sin_port = htons(addr->port);
    addr_in->sin_addr.s_addr = inet_addr(addr->addr);
    // inet_pton(AF_INET, addr->addr, &addr_in->sin_addr);
}

struct sio_socket *sio_socket_create(enum sio_socket_proto proto)
{
    SIO_COND_CHECK_RETURN_VAL(proto < SIO_SOCK_TCP || proto > SIO_SOCK_UDP, NULL);

    struct sio_socket *sock = malloc(sizeof(struct sio_socket));
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!sock, NULL,
        SIO_LOGE("sio_socket malloc failed\n"));

    memset(sock, 0, sizeof(struct sio_socket));

    struct sio_socket_attr *attr = &sock->attr;
    attr->proto = proto;
    
    sock->fd = -1;

    return sock;
}

struct sio_socket *sio_socket_create2(enum sio_socket_proto proto)
{
    struct sio_socket *sock = sio_socket_create(proto);
    SIO_COND_CHECK_RETURN_VAL(!sock, NULL);

    int fd = sio_socket_sock(proto);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(fd == -1, NULL,
        SIO_LOGE("sio_socket socket failed\n"),
        free(sock));

    return sock;
}

int sio_socket_option(struct sio_socket *sock, enum sio_socket_optcmd cmd, union sio_socket_opt *opt)
{
    SIO_COND_CHECK_RETURN_VAL(!sock || !opt, -1);

    int ret = 0;
    switch (cmd) {
    case SIO_SOCK_PRIVATE:
        sio_socket_set_private(sock, opt->private);
        break;

    case SIO_SOCK_OPS:
        sio_socket_set_ops(sock, opt->ops);
        break;

    default:
        break;
    }

    SIO_COND_CHECK_RETURN_VAL(sock->fd == -1, -1);
    switch (cmd) {
    case SIO_SOCK_NONBLOCK:
        ret = sio_socket_set_nonblock(sock, opt->nonblock);
        break;
    
    case SIO_SOCK_REUSEADDR:
        ret = sio_socket_set_addrreuse(sock, opt->reuseaddr);
        break;
    
    default:
        break;
    }

    return ret;
}

int sio_socket_listen(struct sio_socket *sock, struct sio_socket_addr *addr)
{
    SIO_COND_CHECK_RETURN_VAL(!sock || !addr, -1);

    SIO_COND_CHECK_CALLOPS_RETURN_VAL(sock->fd != -1, -1,
        SIO_LOGE("sio_socket is listenning\n"));
    
    struct sio_socket_attr *attr = &sock->attr;
    int fd = sio_socket_sock(attr->proto);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(fd == -1, -1,
        SIO_LOGE("sio_socket socket failed\n"));
    
    struct sockaddr_in addr_in = { 0 };
    sio_socket_addr_in(sock, addr, &addr_in);

    int ret = bind(fd, (struct sockaddr *)&addr_in, sizeof(addr_in));
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        SIO_LOGE("sio_socket bind failed\n"),
        close(fd));

    ret = listen(fd, SOMAXCONN);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        SIO_LOGE("sio_socket listen failed\n"),
        close(fd));

    sock->fd = fd;
    attr->mean = SIO_SOCK_MEAN_SERVER;

    return 0;
}

int sio_socket_accept(struct sio_socket *serv, struct sio_socket *sock)
{
    SIO_COND_CHECK_RETURN_VAL(!serv || !sock, -1);
    SIO_COND_CHECK_RETURN_VAL(serv->fd == -1, -1);

    int fd = accept(serv->fd, NULL, NULL);
    SIO_COND_CHECK_RETURN_VAL(fd == -1, -1);

    sock->fd = fd;

    struct sio_socket_attr *attr = &sock->attr;
    attr->mean = SIO_SOCK_MEAN_SOCKET;

    return 0;
}

int sio_socket_async_accept(struct sio_socket *serv, struct sio_socket *sock)
{
    SIO_COND_CHECK_RETURN_VAL(!serv || !sock, -1);
    SIO_COND_CHECK_RETURN_VAL(serv->fd == -1 || sock->fd == -1 || !sock->mp, -1);

    struct sio_event ev = { 0 };
    ev.events |= SIO_EVENTS_ASYNC_ACCEPT;
    ev.owner.fd = serv->fd;
    ev.owner.ptr = serv;
    ev.buf.ptr = sock->extbuf;
    ev.buf.len = sizeof(sock->extbuf);
    return sio_mplex_ctl(sock->mp, SIO_EV_OPT_ADD, sock->fd, &ev);
}

#ifdef LINUX
#define sio_socket_connect_err_check()                              \
    errno != EINPROGRESS;
#elif defined(WIN32)
#define sio_socket_connect_err_check()                              \
    WSAGetLastError() != WSAEWOULDBLOCK;
#endif

int sio_socket_connect(struct sio_socket *sock, struct sio_socket_addr *addr)
{
    SIO_COND_CHECK_RETURN_VAL(!sock || !addr, -1);
    SIO_COND_CHECK_RETURN_VAL(sock->fd != -1, -1);

    struct sio_socket_attr *attr = &sock->attr;
    int fd = sio_socket_sock(attr->proto);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(fd == -1, -1,
        SIO_LOGE("sio_socket socket failed\n"));

    struct sockaddr_in addr_in = { 0 };
    sio_socket_addr_in(sock, addr, &addr_in);

    int ret = connect(fd, (struct sockaddr *)&addr, sizeof(addr_in));
    int err = sio_socket_connect_err_check();
    
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1 && err == 1, -1, 
        close(fd));

    sock->fd = fd;
    attr->mean = SIO_SOCK_MEAN_SOCKET;

    return 0;
}

int sio_socket_read(struct sio_socket *sock, char *buf, int maxlen)
{
    SIO_COND_CHECK_RETURN_VAL(!sock || sock->fd == -1, -1);
    SIO_COND_CHECK_RETURN_VAL(!buf || maxlen == 0, -1);

    int ret = recv(sock->fd, buf, maxlen, 0);
    // SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
    //     SIO_LOGE("errno: %d\n", errno));

    return ret;
}

int sio_socket_async_read(struct sio_socket *sock, char *buf, int maxlen)
{
    SIO_COND_CHECK_RETURN_VAL(!sock || sock->fd == -1 || !sock->mp, -1);
    SIO_COND_CHECK_RETURN_VAL(!buf || maxlen == 0, -1);

    struct sio_event ev = { 0 };
    ev.events |= SIO_EVENTS_ASYNC_READ;
    ev.owner.fd = sock->fd;
    ev.owner.ptr = sock;
    ev.buf.ptr = buf;
    ev.buf.len = maxlen;
    return sio_mplex_ctl(sock->mp, SIO_EV_OPT_ADD, sock->fd, &ev);
}

int sio_socket_write(struct sio_socket *sock, char *buf, int len)
{
    SIO_COND_CHECK_RETURN_VAL(!sock || sock->fd == -1, -1);
    SIO_COND_CHECK_RETURN_VAL(!buf || len == 0, -1);

    return send(sock->fd, buf, len, 0);
}

int sio_socket_async_write(struct sio_socket *sock, char *buf, int len)
{
    SIO_COND_CHECK_RETURN_VAL(!sock || sock->fd == -1 || !sock->mp, -1);
    SIO_COND_CHECK_RETURN_VAL(!buf || len == 0, -1);

    struct sio_event ev = { 0 };
    ev.events |= SIO_EVENTS_ASYNC_WRITE;
    ev.owner.fd = sock->fd;
    ev.owner.ptr = sock;
    ev.buf.ptr = buf;
    ev.buf.len = len;
    return sio_mplex_ctl(sock->mp, SIO_EV_OPT_ADD, sock->fd, &ev);
}

int sio_socket_mplex_bind(struct sio_socket *sock, struct sio_mplex *mp)
{
    SIO_COND_CHECK_RETURN_VAL(!sock || !mp, -1);

    sock->mp = mp;
    return 0;
}

int sio_socket_mplex(struct sio_socket *sock, enum SIO_EVENTS_OPT op, enum SIO_EVENTS events)
{
    SIO_COND_CHECK_RETURN_VAL(!sock, -1);
    SIO_COND_CHECK_RETURN_VAL(sock->fd == -1 || !sock->mp, -1);

    if (sock->attr.nonblock) {
        events |= SIO_EVENTS_ET;
    }

    struct sio_event ev = { 0 };
    ev.events |= events;
    ev.owner.fd = sock->fd;
    ev.owner.ptr = sock;
    return sio_mplex_ctl(sock->mp, op, sock->fd, &ev);
}

void *sio_socket_private(struct sio_socket *sock)
{
    SIO_COND_CHECK_RETURN_VAL(!sock, NULL);

    return sock->owner.uptr;
}

int sio_socket_close(struct sio_socket *sock)
{
    SIO_COND_CHECK_RETURN_VAL(!sock, -1);
    SIO_COND_CHECK_RETURN_VAL(sock->fd == -1, -1);

    CLOSE(sock->fd);

    return 0;
}

int sio_socket_destory(struct sio_socket *sock)
{
    SIO_COND_CHECK_RETURN_VAL(!sock, -1);
    free(sock);
    return 0;
}
