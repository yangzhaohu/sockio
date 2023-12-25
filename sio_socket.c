#include "sio_socket.h"
#include <string.h>
#include <stdlib.h>
#ifndef WIN32
#include <fcntl.h>
#endif
#include "sio_common.h"
#include "sio_socket_pri.h"
#include "sio_sslctx.h"
#include "sio_sockssl.h"
#include "sio_event.h"
#include "sio_mplex.h"
#include "sio_aio.h"
#include "sio_errno.h"
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
    enum sio_sockprot prot;
    enum sio_socket_mean mean;
    int nonblock;
};

struct sio_socket_state
{
    // enum sio_events events;
    enum sio_socksh shut;
    enum sio_sockwhat what;
    enum sio_events events;
    enum sio_events levents;
    int placement:1;
    int listening:1;
    int connected:1;
    int dup:1;
};

struct sio_socket_owner
{
    void *pri;
    struct sio_sockops ops;
};

struct sio_socket
{
    sio_fd_t fd;
    struct
    {
        sio_sslctx_t ctx;
        struct sio_sockssl *sock;
    } ssl;

    struct sio_socket_state stat;
    struct sio_socket_attr attr;
    struct sio_socket_owner owner;
    struct sio_mplex *mp;
    char extbuf[128];      // extension buffer, additional purposes, such as acceptex of iocp
};

#ifdef WIN32
__declspec(thread) int tls_accept_pend_fd = -1;
__declspec(thread) int tls_sock_readerr = 0;
#else
__thread int tls_accept_pend_fd = -1;
__thread int tls_sock_readerr = 0;
#endif

#define sio_socket_accept_pend_fd()     tls_accept_pend_fd
#define sio_socket_accept_pend_fd_set(fd)   tls_accept_pend_fd = fd
#define sio_socket_accept_pend_fd_clear()   tls_accept_pend_fd = -1

#define sio_socket_readerr()     tls_sock_readerr
#define sio_socket_readerr_set(err)   tls_sock_readerr = err
#define sio_socket_readerr_clear()   tls_sock_readerr = 0

#define SIO_SOCK_EXT_TO_AIOBUF(ptr) (char *)ptr + sio_aioctx_size()
#define SIO_SOCK_AIOBUF_TO_EXT(ptr) (char *)ptr - sio_aioctx_size()

#ifdef WIN32
#define SIO_SOCK_SHUT_RD SD_RECEIVE
#define SIO_SOCK_SHUT_WR SD_SEND
#define SIO_SOCK_SHUT_RDWR SD_BOTH
#else
#define SIO_SOCK_SHUT_RD SHUT_RD
#define SIO_SOCK_SHUT_WR SHUT_WR
#define SIO_SOCK_SHUT_RDWR SHUT_RDWR
#endif

#ifdef WIN32
#define sio_socket_eagain(err) (err == WSAEWOULDBLOCK)
#define sio_socket_enotconn(err) (err == WSAENOTCONN)
#define sio_socket_econnabort(err) (err == WSAECONNABORTED)
#define sio_socket_eshutdown(err) (err == WSAESHUTDOWN)
#define sio_socket_einval(err) (0)
#define sio_socket_enotsock(err) (err == WSAENOTSOCK)
#else
#define sio_socket_eagain(err) (err == EAGAIN || err == EWOULDBLOCK)
#define sio_socket_enotconn(err) (err == ENOTCONN)
#define sio_socket_econnabort(err) (err == ECONNABORTED)
#define sio_socket_eshutdown(err) (err == ESHUTDOWN)
#define sio_socket_einval(err) (err == EINVAL)
#define sio_socket_enotsock(err) (err == ENOTSOCK)
#endif

// shutdown and break loop
#define sio_socket_shutdown_break(fd)               \
    shutdown(fd, SIO_SOCK_SHUTRD);                  \
    break;

// sio_socket ops call
#define sio_socket_ops_call(ops, ...)               \
    if (ops) {                                      \
        ops(__VA_ARGS__);                           \
    }

#define sio_socket_ops_call_break(ops, ...)         \
    sio_socket_ops_call(ops, __VA_ARGS__)           \
    break;

#define sio_socket_state_mplexing_set(sock, val)    \
    sock->stat.mplexing = val

#define sio_socket_state_mplexing_get(sock)         \
    sock->stat.mplexing

//
#define sio_socket_state_shut_set(sock, val)        \
    sock->stat.shut = val

#define sio_socket_state_shut_get(sock)             \
    sock->stat.shut

#define sio_socket_close_cleanup(sock)                                  \
    sio_sock_mplex_event_del(sock);                                     \
    sio_socket_state_shut_set(sock, SIO_SOCK_SHUTRDWR);                 \
    sio_socket_ops_call(ops->closeable, sock);

#define sio_sock_mplex_event_del(sock)                          \
    do {                                                        \
        stat->levents = 0;                                      \
        struct sio_event event = { 0 };                         \
        sio_mplex_ctl(sock->mp,                                 \
            SIO_EV_OPT_DEL, sock->fd, &event);                  \
    } while (0);

#define sio_sockssl_handshake2(sock)                                    \
    do {                                                                \
        int err = sio_sockssl_handshake(sock->ssl.sock);                \
        if (err == SIO_SOCKSSL_EWANTREAD) {                             \
            stat->levents = SIO_EVENTS_IN;                              \
        } else if (err == SIO_SOCKSSL_EWANTWRITE) {                     \
            stat->levents = SIO_EVENTS_OUT;                             \
        } else if (err == 0) {                                          \
            stat->what = SIO_SOCK_ESTABLISHED;                          \
            stat->levents = 0;                                          \
            sio_socket_ops_call(ops->handshaked, sock);                 \
        } else {                                                        \
            SIO_LOGI("handshake err: %d\n", err);                       \
            stat->levents = 0;                                          \
            stat->what = SIO_SOCK_OPEN;                                 \
        }                                                               \
        sio_socket_mplex_imp(sock, SIO_EV_OPT_MOD,                      \
            stat->events | stat->levents);                              \
    } while(0);

#define sio_sockssl_async_handshake(sockpri, sslops)                        \
    do {                                                                    \
        int err = sslops(sockpri->ssl.sock);                                \
        if (err == SIO_SOCKSSL_EWANTREAD) {                                 \
            sio_socket_async_handshake_read(sockpri);                       \
        } else if (err == SIO_SOCKSSL_EWANTREAD) {                          \
            sio_socket_async_handshake_write(sockpri);                      \
        } else if (err == 0) {                                              \
            sockpri->stat.what = SIO_SOCK_ESTABLISHED;                      \
            sio_sockssl_enable_membio(sockpri->ssl.sock);                   \
            sio_socket_ops_call(sockpri->owner.ops.handshaked, sockpri);    \
        }                                                                   \
    } while (0);


#define sio_socket_socket_recv(sock)                                        \
    do {                                                                    \
        if (stat->what == SIO_SOCK_HANDSHAKE) {                             \
            sio_sockssl_handshake2(sock);                                   \
            break;                                                          \
        }                                                                   \
        sio_socket_ops_call(ops->readable, sock);                           \
        int err = sio_socket_readerr();                                     \
        sio_socket_readerr_clear();                                         \
        SIO_COND_CHECK_BREAK(sio_socket_eagain(err));                       \
        if (sio_socket_enotconn(err) |                                      \
            sio_socket_econnabort(err) |                                    \
            sio_socket_eshutdown(err) |                                     \
            sio_socket_einval(err) |                                        \
            sio_socket_enotsock(err)) {                                     \
            sio_sock_mplex_event_del(sock);                                 \
            sio_socket_state_shut_set(sock, SIO_SOCK_SHUTRDWR);             \
            sio_socket_ops_call(ops->closeable, sock);                      \
            break;                                                          \
        } else if (err != 0) {                                              \
            SIO_LOGI("pending errno: %d, break\n", err);                    \
            break;                                                          \
        }                                                                   \
    } while (attr->nonblock);

// dispatch once
#define sio_socket_event_dispatch_once(event)                                   \
    if (event->events & (SIO_EVENTS_IN | SIO_EVENTS_HUP)) {                     \
        if (event->events & SIO_EVENTS_HUP) {                                   \
            sio_sock_mplex_event_del(sock);                                     \
            sio_socket_state_shut_set(sock, SIO_SOCK_SHUTRDWR);                 \
            sio_socket_ops_call(ops->closeable, sock);                          \
            continue;                                                           \
        }                                                                       \
        sio_socket_socket_recv(sock);                                           \
    }                                                                           \
    if (event->events & SIO_EVENTS_ASYNC_READ) {                                \
        if (attr->mean == SIO_SOCK_MEAN_SOCKET                                  \
            && event->buf.len == 0) {                                           \
            sio_socket_close_cleanup(sock);                                     \
            continue;                                                           \
        }                                                                       \
        if (sock->attr.prot == SIO_SOCK_SSL) {                                  \
            int len = sio_sockssl_writeto_rbio(sock->ssl.sock,                  \
                event->buf.ptr, event->buf.len);                                \
            len = sio_sockssl_read(sock->ssl.sock,                              \
                event->buf.ptr, event->buf.len);                                \
            event->buf.len = len;                                               \
        }                                                                       \
        sio_socket_ops_call(ops->readasync,                                     \
            sock, event->buf.ptr, event->buf.len);                              \
    }                                                                           \
    if (event->events & SIO_EVENTS_ASYNC_WRITE) {                               \
        sio_socket_ops_call(ops->writeasync,                                    \
            sock, event->buf.ptr, event->buf.len);                              \
    }                                                                           \
    if (event->events & SIO_EVENTS_ASYNC_ACCEPT) {                              \
        char *__extptr = SIO_SOCK_AIOBUF_TO_EXT(event->buf.ptr);                \
        struct sio_socket *__sock =                                             \
        SIO_CONTAINER_OF(__extptr,                                              \
            struct sio_socket,                                                  \
            extbuf);                                                            \
        sio_socket_ops_call(ops->acceptasync,                                   \
            sock, __sock);                                                      \
        if (__sock->attr.prot == SIO_SOCK_SSL) {                                \
            __sock->stat.what = SIO_SOCK_HANDSHAKE;                             \
            sio_sockssl_async_handshake(__sock, sio_sockssl_accept);            \
        }                                                                       \
    }                                                                           \
    if (event->events & (SIO_EVENTS_ASYNC_HANDSHAKE_READ |                      \
        SIO_EVENTS_ASYNC_HANDSHAKE_WRITE)) {                                    \
        sio_sockssl_async_handshake(sock, sio_sockssl_handshake);               \
        continue;                                                               \
    }                                                                           \
    if (event->events & SIO_EVENTS_OUT) {                                       \
        if (stat->what == SIO_SOCK_CONNECT) {                                   \
            int errval = 0;                                                     \
            socklen_t errl = sizeof(int);                                       \
            int ret = getsockopt(sock->fd,                                      \
                SOL_SOCKET, SO_ERROR, (void *)&errval, &errl);                  \
            if (ret == 0 && errval == 0) {                                      \
                if (attr->prot == SIO_SOCK_SSL) {                               \
                    stat->what = SIO_SOCK_HANDSHAKE;                            \
                    sio_socket_ops_call(ops->connected,                         \
                        sock, SIO_SOCK_ESTABLISHED);                            \
                } else {                                                        \
                    stat->what = SIO_SOCK_ESTABLISHED;                          \
                    stat->levents = 0;                                          \
                    sio_socket_ops_call(ops->connected,                         \
                        sock, SIO_SOCK_ESTABLISHED);                            \
                }                                                               \
            } else {                                                            \
                stat->what = SIO_SOCK_OPEN;                                     \
                stat->levents = 0;                                              \
                sio_socket_ops_call(ops->connected,                             \
                    sock, SIO_SOCK_OPEN);                                       \
            }                                                                   \
            sio_socket_mplex_imp(sock, SIO_EV_OPT_MOD,                          \
                stat->events | stat->levents);                                  \
        } else if (stat->what == SIO_SOCK_HANDSHAKE) {                          \
            sio_sockssl_handshake2(sock);                                       \
        } else if (stat->what == SIO_SOCK_ESTABLISHED) {                        \
            sio_socket_ops_call(ops->writeable, sock);                          \
        }                                                                       \
    }


static inline
int sio_socket_mplex_imp(struct sio_socket *sock, enum sio_events_opt op, enum sio_events events);

static inline
int sio_socket_async_handshake_read(struct sio_socket *sock)
{
    struct sio_event ev = { 0 };
    ev.events |= SIO_EVENTS_ASYNC_HANDSHAKE_READ;
    ev.pri = sock;
    ev.buf.ptr = SIO_SOCK_EXT_TO_AIOBUF(sock->extbuf);
    ev.buf.len = 0;

    return sio_mplex_ctl(sock->mp, SIO_EV_OPT_ADD, sock->fd, &ev);
}

static inline
int sio_socket_async_handshake_write(struct sio_socket *sock)
{
    struct sio_event ev = { 0 };
    ev.events |= SIO_EVENTS_ASYNC_HANDSHAKE_WRITE;
    ev.pri = sock;
    ev.buf.ptr = SIO_SOCK_EXT_TO_AIOBUF(sock->extbuf);
    ev.buf.len = 0;

    return sio_mplex_ctl(sock->mp, SIO_EV_OPT_ADD, sock->fd, &ev);
}

extern int sio_socket_event_dispatch(struct sio_event *events, int count)
{
    for (int i = 0; i < count; i++) {
        struct sio_event *event = &events[i];
        struct sio_socket *sock = event->pri;
        if (sock == NULL) {
            continue;
        }
        struct sio_socket_attr *attr = &sock->attr;
        struct sio_socket_state *stat = &sock->stat;
        struct sio_socket_owner *owner = &sock->owner;
        struct sio_sockops *ops = &owner->ops;
        // SIO_LOGI("socket fd: %d, event: 0x%x\n", sock->fd, event->events);

        sio_socket_event_dispatch_once(event);
    }

    return 0;
}

static inline
sio_fd_t sio_socket_sock(enum sio_sockprot prot)
{
    sio_fd_t fd;
    if (prot == SIO_SOCK_TCP || prot == SIO_SOCK_SSL) {
        fd = socket(PF_INET, SOCK_STREAM, 0);
    } else if (prot == SIO_SOCK_UDP) {
        fd = socket(PF_INET, SOCK_DGRAM, 0);
    } else {
        fd = -1;
    }

    return fd;
}

static inline 
unsigned short sio_socket_domain(enum sio_sockprot prot)
{
    unsigned short domain = PF_INET;
    if (SIO_SOCK_TCP <= prot && SIO_SOCK_SSL <= prot) {
        domain = PF_INET;
    }

    return domain;
}

static inline
void sio_socket_set_private(struct sio_socket *sock, void *private)
{
    struct sio_socket_owner *owner = &sock->owner;
    owner->pri = private;
}

static inline
void sio_socket_get_private(struct sio_socket *sock, void **private)
{
    struct sio_socket_owner *owner = &sock->owner;
    *private = owner->pri;
}

static inline
void sio_socket_set_ops(struct sio_socket *sock, struct sio_sockops *ops)
{
    struct sio_socket_owner *owner = &sock->owner;

    owner->ops.connected = ops->connected;
    owner->ops.handshaked = ops->handshaked;
    owner->ops.readable = ops->readable;
    owner->ops.writeable = ops->writeable;
    owner->ops.acceptasync = ops->acceptasync;
    owner->ops.readasync = ops->readasync;
    owner->ops.writeasync = ops->writeasync;
    owner->ops.closeable = ops->closeable;
}

static inline
void sio_socket_get_ops(struct sio_socket *sock, struct sio_sockops *ops)
{
    struct sio_socket_owner *owner = &sock->owner;

    ops->connected = owner->ops.connected;
    ops->handshaked = owner->ops.handshaked;
    ops->readable = owner->ops.readable;
    ops->writeable = owner->ops.writeable;
    ops->acceptasync = owner->ops.acceptasync;
    ops->readasync = owner->ops.readasync;
    ops->writeasync = owner->ops.writeasync;
    ops->closeable = owner->ops.closeable;
}

static inline
void sio_socket_set_mplex(struct sio_socket *sock, struct sio_mplex *mplex)
{
    sock->mp = mplex;
}

static inline
int sio_socket_set_nonblock(struct sio_socket *sock, int nonblock)
{
    SIO_COND_CHECK_RETURN_VAL(sock->fd == -1, -1);

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
    SIO_COND_CHECK_RETURN_VAL(sock->fd == -1, -1);

#ifdef LINUX
#elif defined(WIN32)
    char reused = reuse ? 1 : 0;
    int ret = setsockopt(sock->fd, SOL_SOCKET, SO_REUSEADDR, &reused, sizeof(char));
    SIO_COND_CHECK_RETURN_VAL(ret == SOCKET_ERROR, -1);

#endif
    return 0;
}

static inline
int sio_socket_set_buffsize(struct sio_socket *sock, int size, enum sio_sockoptc cmd)
{
    SIO_COND_CHECK_RETURN_VAL(sock->fd == -1, -1);

#ifdef LINUX
    int optname = cmd == SIO_SOCK_RCVBUF ? SO_RCVBUF : SO_SNDBUF;
    int ret = setsockopt(sock->fd, SOL_SOCKET, optname, &size, sizeof(size));
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    // socklen_t sent = sizeof(size);
    // size = 0;
    // ret = getsockopt(sock->fd, SOL_SOCKET, SO_SNDBUF, &size, &sent);
    // SIO_LOGI("sent buf: %d\n", size);
    // SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);
#else
    int ret = -1;
#endif

    return ret;
}

static inline
int sio_socket_set_sock_timeout(struct sio_socket *sock, int ms, enum sio_sockoptc cmd)
{
    int optname = cmd == SIO_SOCK_SNDTIMEO ? SO_SNDTIMEO : SO_RCVTIMEO;
#ifdef WIN32
    int ret = setsockopt(sock->fd, SOL_SOCKET, optname, (void *)&ms, sizeof(ms));
#else
    struct timeval timeout = {
        .tv_sec = ms / 1000,
        .tv_usec = (ms % 1000) * 1000
    };
    int ret = setsockopt(sock->fd, SOL_SOCKET, optname, &timeout, sizeof(timeout));
#endif

    return ret;
}

static inline 
void sio_socket_addr_in(struct sio_socket *sock, struct sio_sockaddr *addr, struct sockaddr_in *addr_in)
{
    unsigned short domain = sio_socket_domain(sock->attr.prot);
    
    addr_in->sin_family = domain;
    addr_in->sin_port = htons(addr->port);
    addr_in->sin_addr.s_addr = inet_addr(addr->addr);
    // inet_pton(AF_INET, addr->addr, &addr_in->sin_addr);
}

static inline
void sio_socket_destory_imp(struct sio_socket *sock)
{
    SIO_COND_CHECK_CALLOPS(sock->ssl.sock,
        sio_sockssl_destory(sock->ssl.sock));

    SIO_COND_CHECK_CALLOPS(sock->ssl.ctx,
        sio_sslctx_destory(sock->ssl.ctx));

    SIO_COND_CHECK_CALLOPS(sock->stat.placement,
        free(sock));
}

static inline
struct sio_socket *sio_socket_alloc(enum sio_sockprot prot, char *placement)
{
    struct sio_socket *sock = (struct sio_socket *)placement;
    if (sock == NULL) {
        sock = malloc(sizeof(struct sio_socket));
    }
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!sock, NULL,
        SIO_LOGE("sio_socket malloc failed\n"));

    memset(sock, 0, sizeof(struct sio_socket));

    struct sio_socket_state *stat = &sock->stat;
    stat->placement = placement == NULL ? 1 : 0;
    stat->what = SIO_SOCK_NONE;

    struct sio_socket_attr *attr = &sock->attr;
    attr->prot = prot;
    attr->mean = SIO_SOCK_MEAN_SOCKET;
    
    sock->fd = -1;

    return sock;
}

static inline
struct sio_socket *sio_socket_create_imp(enum sio_sockprot prot, char *placement)
{
    struct sio_socket *sock = sio_socket_alloc(prot, placement);
    SIO_COND_CHECK_RETURN_VAL(!sock, NULL);

    if (prot == SIO_SOCK_SSL) {
        sock->ssl.ctx = sio_sslctx_create();
        SIO_COND_CHECK_CALLOPS_RETURN_VAL(!sock->ssl.ctx, NULL,
            sio_socket_destory_imp(sock));
    }

    return sock;
}

unsigned int sio_socket_struct_size()
{
    return sizeof(struct sio_socket);
}

struct sio_socket *sio_socket_create(enum sio_sockprot prot, char *placement)
{
    SIO_COND_CHECK_RETURN_VAL(prot < SIO_SOCK_TCP || prot > SIO_SOCK_SSL, NULL);

    return sio_socket_create_imp(prot, placement);
}

struct sio_socket *sio_socket_create2(enum sio_sockprot prot, char *placement)
{
    SIO_COND_CHECK_RETURN_VAL(prot < SIO_SOCK_TCP || prot > SIO_SOCK_SSL, NULL);

    struct sio_socket *sock = sio_socket_create_imp(prot, placement);
    SIO_COND_CHECK_RETURN_VAL(!sock, NULL);

    sio_fd_t fd = sio_socket_sock(prot);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(fd == -1, NULL,
        SIO_LOGE("socket failed\n"),
        sio_socket_destory_imp(sock));

    sock->fd = fd;
    if (prot == SIO_SOCK_SSL) {
        sock->ssl.sock = sio_sockssl_create(sock->ssl.ctx);
        SIO_COND_CHECK_CALLOPS_RETURN_VAL(!sock->ssl.sock, NULL,
            sio_socket_destory_imp(sock));
    }

    struct sio_socket_state *stat = &sock->stat;
    stat->what = SIO_SOCK_OPEN;

    return sock;
}

struct sio_socket *sio_socket_dup(struct sio_socket *sock, char *placement)
{
    enum sio_sockprot prot = sock->attr.prot;
    struct sio_socket *sdup = sio_socket_alloc(prot, placement);
    SIO_COND_CHECK_RETURN_VAL(!sdup, NULL);

    if (prot == SIO_SOCK_SSL) {
        sdup->ssl.sock = sio_sockssl_create(sock->ssl.ctx);
        SIO_COND_CHECK_CALLOPS_RETURN_VAL(!sdup->ssl.sock, NULL,
            sio_socket_destory_imp(sdup));
    }

    struct sio_socket_attr *attr = &sdup->attr;
    attr->nonblock = sock->attr.nonblock;

    struct sio_socket_state *stat = &sdup->stat;
    stat->dup = 1;

    return sdup;
}

struct sio_socket *sio_socket_dup2(struct sio_socket *sock, char *placement)
{
    enum sio_sockprot prot = sock->attr.prot;
    struct sio_socket *sdup = sio_socket_alloc(prot, placement);
    SIO_COND_CHECK_RETURN_VAL(!sdup, NULL);

    sio_fd_t fd = sio_socket_sock(prot);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(fd == -1, NULL,
        SIO_LOGE("socket failed\n"),
        sio_socket_destory_imp(sock));

    sdup->fd = fd;
    if (prot == SIO_SOCK_SSL) {
        sdup->ssl.sock = sio_sockssl_create(sock->ssl.ctx);
        SIO_COND_CHECK_CALLOPS_RETURN_VAL(!sdup->ssl.sock, NULL,
            sio_socket_destory_imp(sdup));

        int ret = sio_sockssl_setfd(sdup->ssl.sock, fd);
        SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, NULL,
            sio_socket_destory_imp(sdup),
            sio_sockssl_destory(sdup->ssl.sock));
    }

    struct sio_socket_attr *attr = &sdup->attr;
    attr->nonblock = sock->attr.nonblock;
    if (attr->nonblock) {
        sio_socket_set_nonblock(sdup, 1);
    }

    struct sio_socket_state *stat = &sdup->stat;
    stat->what = SIO_SOCK_OPEN;
    stat->dup = 1;

    return sdup;
}

int sio_socket_setopt(struct sio_socket *sock, enum sio_sockoptc cmd, union sio_sockopt *opt)
{
    SIO_COND_CHECK_RETURN_VAL(!sock || !opt, -1);

    int ret = 0;
    switch (cmd) {
    case SIO_SOCK_PRIVATE:
        sio_socket_set_private(sock, opt->private);
        break;

    case SIO_SOCK_OPS:
        sio_socket_set_ops(sock, &opt->ops);
        break;

    case SIO_SOCK_MPLEX:
        sio_socket_set_mplex(sock, opt->mplex);
        break;

    case SIO_SOCK_NONBLOCK:
        ret = sio_socket_set_nonblock(sock, opt->nonblock);
        break;

    case SIO_SOCK_REUSEADDR:
        ret = sio_socket_set_addrreuse(sock, opt->reuseaddr);
        break;

    case SIO_SOCK_RCVBUF:
        ret = sio_socket_set_buffsize(sock, opt->rcvbuf, cmd);
        break;
    case SIO_SOCK_SNDBUF:
        ret = sio_socket_set_buffsize(sock, opt->sndbuf, cmd);
        break;

    case SIO_SOCK_SNDTIMEO:
    case SIO_SOCK_RCVTIMEO:
        ret = sio_socket_set_sock_timeout(sock, opt->timeout, cmd);
        break;

    case SIO_SOCK_SSL_CACERT:
    {
        ret = -1;
        union sio_sslopt sopt = { .data = opt->data };
        if (sock->stat.dup) {
            SIO_COND_CHECK_BREAK(!sock->ssl.sock);
            ret = sio_sockssl_setopt(sock->ssl.sock, SIO_SSL_CACERT, &sopt);
        } else {
            SIO_COND_CHECK_CALLOPS(sock->ssl.ctx,
                ret = sio_sslctx_setopt(sock->ssl.ctx, SIO_SSL_CACERT, &sopt));
            SIO_COND_CHECK_CALLOPS(sock->ssl.sock,
                ret = sio_sockssl_setopt(sock->ssl.sock, SIO_SSL_CACERT, &sopt));
        }
        break;
    }
    case SIO_SOCK_SSL_USERCERT:
    {
        ret = -1;
        union sio_sslopt sopt = { .data = opt->data };
        if (sock->stat.dup) {
            SIO_COND_CHECK_BREAK(!sock->ssl.sock);
            ret = sio_sockssl_setopt(sock->ssl.sock, SIO_SSL_USERCERT, &sopt);
        } else {
            SIO_COND_CHECK_CALLOPS(sock->ssl.ctx,
                ret = sio_sslctx_setopt(sock->ssl.ctx, SIO_SSL_USERCERT, &sopt));
            SIO_COND_CHECK_CALLOPS(sock->ssl.sock,
                ret = sio_sockssl_setopt(sock->ssl.sock, SIO_SSL_USERCERT, &sopt));
        }
        break;
    }
    case SIO_SOCK_SSL_USERKEY:
    {
        ret = -1;
        union sio_sslopt sopt = { .data = opt->data };
        if (sock->stat.dup) {
            SIO_COND_CHECK_BREAK(!sock->ssl.sock);
            ret = sio_sockssl_setopt(sock->ssl.sock, SIO_SSL_USERKEY, &sopt);
        } else {
            SIO_COND_CHECK_CALLOPS(sock->ssl.ctx,
                ret = sio_sslctx_setopt(sock->ssl.ctx, SIO_SSL_USERKEY, &sopt));
            SIO_COND_CHECK_CALLOPS(sock->ssl.sock,
                ret = sio_sockssl_setopt(sock->ssl.sock, SIO_SSL_USERKEY, &sopt));
        }
        break;
    }
    case SIO_SOCK_SSL_VERIFY_PEER:
    {
        ret = -1;
        union sio_sslopt sopt = { .enable = opt->enable };
        if (sock->stat.dup) {
            SIO_COND_CHECK_BREAK(!sock->ssl.sock);
            ret = sio_sockssl_setopt(sock->ssl.sock, SIO_SSL_VERIFY_PEER, &sopt);
        } else {
            SIO_COND_CHECK_CALLOPS(sock->ssl.ctx,
                ret = sio_sslctx_setopt(sock->ssl.ctx, SIO_SSL_VERIFY_PEER, &sopt));
            SIO_COND_CHECK_CALLOPS(sock->ssl.sock,
                ret = sio_sockssl_setopt(sock->ssl.sock, SIO_SSL_VERIFY_PEER, &sopt));
        }
        break;
    }

    default:
        ret = -1;
        break;
    }

    return ret;
}

int sio_socket_getopt(struct sio_socket *sock, enum sio_sockoptc cmd, union sio_sockopt *opt)
{
    SIO_COND_CHECK_RETURN_VAL(!sock || !opt, -1);

    int ret = 0;
    switch (cmd) {
    case SIO_SOCK_PRIVATE:
        sio_socket_get_private(sock, &opt->private);
        break;

    case SIO_SOCK_OPS:
        sio_socket_get_ops(sock, &opt->ops);
        break;
    
    case SIO_SOCK_MPLEX:
        opt->mplex = sock->mp;
        break;

    default:
        ret = -1;
        break;
    }

    return ret;
}

int sio_socket_peername(struct sio_socket *sock, struct sio_sockaddr *peer)
{
    SIO_COND_CHECK_RETURN_VAL(!sock || !peer, -1);

    struct sockaddr_in addr_in = { 0 };
    unsigned int l = sizeof(addr_in);
    int ret = getpeername(sock->fd, (struct sockaddr *)&addr_in, &l);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    inet_ntop(AF_INET, &addr_in.sin_addr.s_addr, peer->addr, sizeof(peer->addr));
    peer->port = ntohs(addr_in.sin_port);

    return 0;
}

int sio_socket_sockname(struct sio_socket *sock, struct sio_sockaddr *addr)
{
    SIO_COND_CHECK_RETURN_VAL(!sock || !addr, -1);

    struct sockaddr_in addr_in = { 0 };
    unsigned int l = sizeof(addr_in);
    int ret = getsockname(sock->fd, (struct sockaddr *)&addr_in, &l);
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    inet_ntop(AF_INET, &addr_in.sin_addr.s_addr, addr->addr, sizeof(addr->addr));
    addr->port = ntohs(addr_in.sin_port);

    return 0;
}

static inline
int sio_socket_bind_imp(struct sio_socket *sock, int fd, struct sio_sockaddr *addr)
{
    struct sockaddr_in addr_in = { 0 };
    sio_socket_addr_in(sock, addr, &addr_in);

    int ret = bind(fd, (struct sockaddr *)&addr_in, sizeof(addr_in));
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

    return 0;
}

int sio_socket_bind(struct sio_socket *sock, struct sio_sockaddr *addr)
{
    SIO_COND_CHECK_RETURN_VAL(!sock || !addr, -1);

    struct sio_socket_attr *attr = &sock->attr;
    sio_fd_t fd = -1;
    if (sock->fd == -1) {
        fd = sio_socket_sock(attr->prot);
        SIO_COND_CHECK_CALLOPS_RETURN_VAL(fd == -1, -1,
            SIO_LOGE("socket socket failed\n"));
    }

    int ret = sio_socket_bind_imp(sock, fd, addr);
       SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        SIO_LOGE("socket bind failed\n"),
        close(fd));

    sock->fd = fd;

    return ret;
}

int sio_socket_listen(struct sio_socket *sock, struct sio_sockaddr *addr)
{
    SIO_COND_CHECK_RETURN_VAL(!sock || !addr, -1);

    struct sio_socket_state *stat = &sock->stat;

    SIO_COND_CHECK_CALLOPS_RETURN_VAL(stat->listening == 1, -1,
        SIO_LOGE("socket is listenning\n"));
    
    struct sio_socket_attr *attr = &sock->attr;
    sio_fd_t fd = sock->fd;
    if (fd == -1) {
        fd = sio_socket_sock(attr->prot);
        SIO_COND_CHECK_CALLOPS_RETURN_VAL(fd == -1, -1,
            SIO_LOGE("socket socket failed\n"));
    }
    
    int ret = sio_socket_bind_imp(sock, fd, addr);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        SIO_LOGE("socket bind failed\n"),
        close(fd));

    ret = listen(fd, SOMAXCONN);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        SIO_LOGE("socket listen failed\n"),
        close(fd));

    sock->fd = fd;
    attr->mean = SIO_SOCK_MEAN_SERVER;

    stat->listening = 1;

    return 0;
}

int sio_socket_accept_has_pend_imp(struct sio_socket *sock)
{
    SIO_COND_CHECK_RETURN_VAL(sock->fd == -1, -1);
    SIO_COND_CHECK_RETURN_VAL(sock->attr.mean != SIO_SOCK_MEAN_SERVER, -1);

    SIO_COND_CHECK_RETURN_VAL(sio_socket_accept_pend_fd() != -1, 0);

    sio_fd_t fd = accept(sock->fd, NULL, NULL);
    // SIO_LOGI("accept fd: %d, err: %d\n", fd, sio_sock_errno);
    if (fd == -1) {
        int err = sio_sock_errno;
        sio_socket_readerr_set(err);
        SIO_COND_CHECK_RETURN_VAL(sio_socket_eagain(err), SIO_ERRNO_AGAIN);
        return -1;
    }

    sio_socket_accept_pend_fd_set(fd);

    return 0;
}

int sio_socket_accept_has_pend(struct sio_socket *sock)
{
    SIO_COND_CHECK_RETURN_VAL(!sock, -1);

    return sio_socket_accept_has_pend_imp(sock);
}

int sio_socket_accept(struct sio_socket *sock, struct sio_socket *newsock)
{
    SIO_COND_CHECK_RETURN_VAL(!sock || !newsock, -1);
    SIO_COND_CHECK_RETURN_VAL(sock->fd == -1, -1);

    int ret = sio_socket_accept_has_pend_imp(sock);
    SIO_COND_CHECK_RETURN_VAL(ret != 0, ret);

    struct sio_socket_attr *attr = &newsock->attr;
    attr->mean = SIO_SOCK_MEAN_SOCKET;

    struct sio_socket_state *stat = &newsock->stat;
    stat->what = SIO_SOCK_ESTABLISHED;

    sio_socket_t fd = sio_socket_accept_pend_fd();
    sio_socket_accept_pend_fd_clear();
    newsock->fd = fd;

    if (sock->attr.nonblock == 1) {
        sio_socket_set_nonblock(newsock, 1);
    }

    if (newsock->attr.prot == SIO_SOCK_SSL) {
        if (!newsock->ssl.sock) {
            newsock->ssl.sock = sio_sockssl_create(sock->ssl.ctx);
            SIO_COND_CHECK_RETURN_VAL(!newsock->ssl.sock, -1);
        }

        int ret = sio_sockssl_setfd(newsock->ssl.sock, fd);
        SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

        ret = sio_sockssl_accept(newsock->ssl.sock);
        
        if (ret == SIO_SOCKSSL_EWANTREAD) {
            stat->levents = SIO_EVENTS_IN;
        } else if (ret == SIO_SOCKSSL_EWANTWRITE) {
            stat->levents = SIO_EVENTS_OUT;
        } else if (ret != 0) {
            SIO_LOGE("sockssl accept ret: %d\n", ret);
            close(fd);
            return -1;
        }

        stat->what = SIO_SOCK_HANDSHAKE;

        if (ret != 0 && sock->mp) {
            sio_socket_mplex_imp(newsock, SIO_EV_OPT_ADD, stat->events | stat->levents);
        }
    }

    return 0;
}

int sio_socket_async_accept(struct sio_socket *serv, struct sio_socket *sock)
{
    SIO_COND_CHECK_RETURN_VAL(!serv || !sock, -1);
    SIO_COND_CHECK_RETURN_VAL(serv->fd == -1 || sock->fd == -1, -1);

    struct sio_event ev = { 0 };
    ev.events |= SIO_EVENTS_ASYNC_ACCEPT;
    ev.pri = serv;
    ev.res = sock->fd;
    ev.buf.ptr = SIO_SOCK_EXT_TO_AIOBUF(sock->extbuf);
    ev.buf.len = sizeof(sock->extbuf) - ((char *)ev.buf.ptr - sock->extbuf);

    return sio_mplex_ctl(sock->mp, SIO_EV_OPT_ADD, serv->fd, &ev);
}

#ifdef LINUX
#define sio_socket_connect_err_check()                              \
    sio_sock_errno != EINPROGRESS;
#elif defined(WIN32)
#define sio_socket_connect_err_check()                              \
    sio_sock_errno != WSAEWOULDBLOCK;
#endif

int sio_socket_connect(struct sio_socket *sock, struct sio_sockaddr *addr)
{
    SIO_COND_CHECK_RETURN_VAL(!sock || !addr, -1);

    struct sio_socket_state *stat = &sock->stat;
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(stat->connected == 1, -1,
        SIO_LOGE("socket is already connected\n"));

    struct sio_socket_attr *attr = &sock->attr;
    sio_fd_t fd = sock->fd;
    if (fd == -1) {
        fd = sio_socket_sock(attr->prot);
        SIO_COND_CHECK_CALLOPS_RETURN_VAL(fd == -1, -1,
            SIO_LOGE("socket failed\n"));
    }

    struct sockaddr_in addr_in = { 0 };
    sio_socket_addr_in(sock, addr, &addr_in);

    int ret = connect(fd, (struct sockaddr *)&addr_in, sizeof(addr_in));
    // SIO_COND_CHECK_CALLOPS(ret != 0,
    //     SIO_LOGE("connect err: %d\n", sio_sock_errno));
    int err = sio_socket_connect_err_check();
    
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1 && err == 1, -1, 
        close(fd));

    if (sock->attr.prot == SIO_SOCK_SSL) {
        if (!sock->ssl.sock) {
            sock->ssl.sock = sio_sockssl_create(sock->ssl.ctx);
            SIO_COND_CHECK_RETURN_VAL(!sock->ssl.sock, -1);
        }

        int ret = sio_sockssl_setfd(sock->ssl.sock, fd);
        SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);

        ret = sio_sockssl_connect(sock->ssl.sock);
        SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret != 0 &&
            ret != SIO_SOCKSSL_EWANTREAD &&
            ret != SIO_SOCKSSL_EWANTWRITE, -1,
            close(fd));
    }

    sock->fd = fd;
    attr->mean = SIO_SOCK_MEAN_SOCKET;

    stat->connected = 1;
    stat->what = SIO_SOCK_CONNECT;

    if (ret != 0 && sock->mp) {
        stat->levents = SIO_EVENTS_OUT;
        sio_socket_mplex_imp(sock, SIO_EV_OPT_ADD, stat->events | stat->levents);
    }

    return 0;
}

static inline
int sio_socket_read_imp(struct sio_socket *sock, char *buf, int len, struct sockaddr_in *addr)
{
    unsigned int l = sizeof(struct sockaddr_in);
    return recvfrom(sock->fd, buf, len, 0, (struct sockaddr *)addr, &l);
}

int sio_socket_read(struct sio_socket *sock, char *buf, int len)
{
    SIO_COND_CHECK_RETURN_VAL(!sock || sock->fd == -1, -1);
    SIO_COND_CHECK_RETURN_VAL(!buf || len == 0, -1);

    int ret;
    if (sock->attr.prot != SIO_SOCK_SSL) {
        struct sockaddr_in addr = { 0 };
        ret = sio_socket_read_imp(sock, buf, len, &addr);
    } else {
        ret = sio_sockssl_read(sock->ssl.sock, buf, len);
        /*if (ret == SIO_SOCKSSL_EZERORETURN) {
            ret = sio_sockssl_shutdown(sock->ssl.sock);
        } else */if (ret == SIO_SOCKSSL_ESSL) {
            ret = 0; // close
        }
    }

    // SIO_LOGI("socket read ret: %d, err: %d\n", ret, sio_sock_errno);

    if (ret <= 0) {
        if (ret == 0) {
#ifdef WIN32
            sio_socket_readerr_set(WSAESHUTDOWN);
#else
            sio_socket_readerr_set(ESHUTDOWN);
#endif
            return ret;
        }
        int err = sio_sock_errno;
        sio_socket_readerr_set(err);
        SIO_COND_CHECK_RETURN_VAL(sio_socket_eagain(err), SIO_ERRNO_AGAIN);
    }

    return ret;
}

int sio_socket_readfrom(struct sio_socket *sock, char *buf, int len, struct sio_sockaddr *peer)
{
    SIO_COND_CHECK_RETURN_VAL(!sock || sock->fd == -1, -1);
    SIO_COND_CHECK_RETURN_VAL(!buf || len == 0 || !peer, -1);

    struct sockaddr_in addr = { 0 };
    int ret = sio_socket_read_imp(sock, buf, len, &addr);

    if (ret == -1) {
        int err = sio_sock_errno;
        sio_socket_readerr_set(err);
        SIO_COND_CHECK_RETURN_VAL(sio_socket_eagain(err), SIO_ERRNO_AGAIN);
    }

    inet_ntop(AF_INET, &addr.sin_addr.s_addr, peer->addr, sizeof(peer->addr));
    peer->port = ntohs(addr.sin_port);

    return ret;
}

static inline
int sio_socket_async_read_imp(struct sio_socket *sock, char *buf, int len)
{
    struct sio_event ev = { 0 };
    ev.events |= SIO_EVENTS_ASYNC_READ;
    ev.pri = sock;
    ev.buf.ptr = buf;
    ev.buf.len = len;

    return sio_mplex_ctl(sock->mp, SIO_EV_OPT_ADD, sock->fd, &ev);
}

int sio_socket_async_read(struct sio_socket *sock, char *buf, int len)
{
    SIO_COND_CHECK_RETURN_VAL(!sock || sock->fd == -1, -1);
    SIO_COND_CHECK_RETURN_VAL(!buf || len == 0, -1);

    return sio_socket_async_read_imp(sock, buf, len);
}

int sio_socket_write(struct sio_socket *sock, const char *buf, int len)
{
    SIO_COND_CHECK_RETURN_VAL(!sock || sock->fd == -1, -1);
    SIO_COND_CHECK_RETURN_VAL(!buf || len == 0, -1);

    if (sock->attr.prot == SIO_SOCK_SSL) {
        return sio_sockssl_write(sock->ssl.sock, buf, len);
    }

    return send(sock->fd, buf, len, 0);
}

int sio_socket_writeto(struct sio_socket *sock, const char *buf, int len, struct sio_sockaddr *peer)
{
    SIO_COND_CHECK_RETURN_VAL(!sock || sock->fd == -1, -1);
    SIO_COND_CHECK_RETURN_VAL(!buf || len == 0 || !peer, -1);

    struct sockaddr_in addr = { 0 };
    sio_socket_addr_in(sock, peer, &addr);

    return sendto(sock->fd, buf, len, 0, (const struct sockaddr *)&addr, sizeof(struct sockaddr_in));
}

static inline
int sio_socket_async_write_imp(struct sio_socket *sock, char *buf, int len)
{
    struct sio_event ev = { 0 };
    ev.events |= SIO_EVENTS_ASYNC_WRITE;
    ev.pri = sock;
    ev.buf.ptr = buf;
    ev.buf.len = len;

    return sio_mplex_ctl(sock->mp, SIO_EV_OPT_ADD, sock->fd, &ev);
}

int sio_socket_async_write(struct sio_socket *sock, char *buf, int len)
{
    SIO_COND_CHECK_RETURN_VAL(!sock || sock->fd == -1, -1);
    SIO_COND_CHECK_RETURN_VAL(!buf || len == 0, -1);

    if (sock->attr.prot == SIO_SOCK_SSL) {
        int l = sio_sockssl_write(sock->ssl.sock, buf, len);
        SIO_COND_CHECK_RETURN_VAL(l <= 0, -1);

        int pending = sio_sockssl_wbio_pending(sock->ssl.sock);
        len = sio_sockssl_readfrom_wbio(sock->ssl.sock, buf, pending);
    }

    return sio_socket_async_write_imp(sock, buf, len);
}

static inline
int sio_socket_mplex_imp(struct sio_socket *sock, enum sio_events_opt op, enum sio_events events)
{
    if (sock->attr.nonblock) {
        events |= SIO_EVENTS_ET;
    }

    struct sio_event ev = { 0 };
    ev.events |= events;
    ev.pri = sock;

    return sio_mplex_ctl(sock->mp, op, sock->fd, &ev);
}

int sio_socket_mplex(struct sio_socket *sock, enum sio_events_opt op, enum sio_events events)
{
    SIO_COND_CHECK_RETURN_VAL(!sock, -1);
    SIO_COND_CHECK_RETURN_VAL(sock->fd == -1 || !sock->mp, -1);

    int ret = sio_socket_mplex_imp(sock, op, events | sock->stat.levents);
    SIO_COND_CHECK_CALLOPS(ret == 0,
        sock->stat.events = events);

    return ret;
}

static inline
int sio_socket_shutdown_imp(struct sio_socket *sock, enum sio_socksh how)
{
    SIO_COND_CHECK_RETURN_VAL(sock->fd == -1, 0);
    SIO_COND_CHECK_RETURN_VAL(how < SIO_SOCK_SHUTRD || how > SIO_SOCK_SHUTRDWR, -1);

    enum sio_socksh shut = sio_socket_state_shut_get(sock);
    how = ~shut & how;
    SIO_COND_CHECK_RETURN_VAL(how == 0, 0);

    int shuthow = 0;
    if (how == SIO_SOCK_SHUTRD) {
        shuthow = SIO_SOCK_SHUT_RD;
    } else if (how == SIO_SOCK_SHUTWR) {
        shuthow = SIO_SOCK_SHUT_WR;
    } else {
        shuthow = SIO_SOCK_SHUT_RDWR;
    }

    if ((sock->stat.events & SIO_EVENTS_IN) == 0) {
        sock->stat.levents = SIO_EVENTS_IN;
        sio_socket_mplex_imp(sock, SIO_EV_OPT_MOD,
            sock->stat.events | sock->stat.levents);
    }

    if (sock->ssl.sock && sock->attr.prot == SIO_SOCK_SSL) {
        int ret = sio_sockssl_shutdown(sock->ssl.sock);
        SIO_COND_CHECK_CALLOPS(ret != SIO_SOCKSSL_EWANTREAD &&
            ret != SIO_SOCKSSL_EWANTWRITE,
            SIO_LOGE("ssl shutdown error, ret: %d\n", ret));
    }

    int ret = shutdown(sock->fd, shuthow);
    if (ret == 0) {
        sio_socket_state_shut_set(sock, shut | how);
    }

    return ret;
}

int sio_socket_shutdown(struct sio_socket *sock, enum sio_socksh how)
{
    SIO_COND_CHECK_RETURN_VAL(!sock, -1);

    struct sio_socket_state *stat = &sock->stat;
    int ret = sio_socket_shutdown_imp(sock, how);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1 && stat->what < SIO_SOCK_CONNECT, -1,
        SIO_LOGE("socket shutdown failed, errno: %d\n", sio_sock_errno));

    stat->listening = 0;
    stat->connected = 0;

    return 0;
}

static inline
int sio_socket_close_imp(struct sio_socket *sock)
{
    SIO_COND_CHECK_RETURN_VAL(sock->fd == -1, 0);

    int ret = CLOSE(sock->fd);
    if (ret == 0) {
        sock->fd = -1;
    }

    return ret;
}

int sio_socket_close(struct sio_socket *sock)
{
    SIO_COND_CHECK_RETURN_VAL(!sock, -1);

    return sio_socket_close_imp(sock);
}

int sio_socket_destory(struct sio_socket *sock)
{
    SIO_COND_CHECK_RETURN_VAL(!sock, -1);

    if (sock->fd != -1) {
        struct sio_socket_state *stat = &sock->stat;
        int how = (~stat->shut) & 0x03;
        SIO_COND_CHECK_CALLOPS(how != 0, sio_socket_shutdown_imp(sock, how));

        int ret = sio_socket_close_imp(sock);
        SIO_COND_CHECK_CALLOPS(ret != 0,
            SIO_LOGE("socket close failed, err: %d\n", sio_sock_errno));
    }

    sio_socket_destory_imp(sock);

    return 0;
}
