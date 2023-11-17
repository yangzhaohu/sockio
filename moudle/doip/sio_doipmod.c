#include "sio_doipmod.h"
#include <string.h>
#include <stdlib.h>
#include "sio_common.h"
#include "sio_server.h"
#include "sio_doip_node.h"
#include "sio_doip_discover.h"
#include "sio_doip_handler.h"
#include "sio_list.h"
#include "sio_timer.h"
#include "sio_log.h"
#include "doip_hdr.h"

struct sio_doipmod
{
    unsigned char maxconn;
    unsigned char curconn;
    struct sio_list_head conhead;
};

struct sio_doip_conn
{
    // list entry
    sio_list_node entry;

    struct sio_doipmod *mod;
    struct sio_socket *sock;

    struct {
        char *buf;
        char *pos;
        char *last;
        char *end;
    }array;

    // doip struct
    struct sio_doip_contbl contbl;
};

int sio_doipmod_name(const char **name)
{
    *name = "sio_doip_core_module";
    return 0;
}

int sio_doipmod_type(void)
{
    return SIO_SUBMOD_DOIP;
}

int sio_doipmod_version(const char **version)
{
    *version = "0.0.1";
    return 0;
}

static inline
void *sio_doip_inittimer_routine(void *arg)
{
    SIO_LOGI("inittimer trigger\n");
    struct sio_doip_conn *dcon = (struct sio_doip_conn *)arg;
    struct sio_doip_contbl *contbl = &dcon->contbl;

    sio_doip_conn_set_state(contbl, SIO_DOIP_STAT_FINALIZE);

    return NULL;
}

static inline
void *sio_doip_genetimer_routine(void *arg)
{
    SIO_LOGI("genetimer trigger\n");
    struct sio_doip_conn *dcon = (struct sio_doip_conn *)arg;
    struct sio_doip_contbl *contbl = &dcon->contbl;

    sio_doip_conn_set_state(contbl, SIO_DOIP_STAT_FINALIZE);

    return NULL;
}

static inline
void sio_doip_conhead_add(struct sio_doipmod *doip, struct sio_doip_conn *dcon)
{
    sio_list_add(&dcon->entry, &doip->conhead);
}

static inline
void sio_doip_conhead_del(struct sio_doip_conn *dcon)
{
    sio_list_del(&dcon->entry);
}

static inline
void sio_doip_dconn_array_moveused(struct sio_doip_conn *dcon)
{
    int overmust = dcon->array.last - dcon->array.pos;
    memcpy(dcon->array.buf, dcon->array.pos, overmust);
    dcon->array.pos = dcon->array.buf;
    dcon->array.last = dcon->array.buf + overmust;
}

static inline
int sio_doip_dconn_array_realloc(struct sio_doip_conn *dcon, unsigned int size)
{
    int len = dcon->array.end - dcon->array.buf;
    int overmust = dcon->array.last - dcon->array.pos;

    size = SIO_MAX(size, overmust);

    if (len != size) {
        char *buf = malloc(size);
        SIO_COND_CHECK_RETURN_VAL(!buf, -1);
        memcpy(buf, dcon->array.pos, overmust);

        dcon->array.buf = buf;
        dcon->array.pos = buf;
        dcon->array.last = buf + overmust;
        dcon->array.end = buf + size;
    }

    return 0;
}

static inline
struct sio_doip_conn *sio_doipmod_doipconn_create()
{
    struct sio_doip_conn *dcon = malloc(sizeof(struct sio_doip_conn));
    SIO_COND_CHECK_RETURN_VAL(!dcon, NULL);

    memset(dcon, 0, sizeof(struct sio_doip_conn));

    sio_doip_dconn_array_realloc(dcon, 4096);

    struct sio_doip_contbl *contbl = &dcon->contbl;
    contbl->stat = SIO_DOIP_STAT_INITIALLIZED;

    contbl->inittimer = sio_timer_create(sio_doip_inittimer_routine, dcon);
    contbl->genetimer = sio_timer_create(sio_doip_genetimer_routine, dcon);

    sio_timer_start(contbl->inittimer, SIO_INIT_TIMEROUT_SEC * 1000 * 1000, 0);

    return dcon;
}

static inline
struct sio_doip_conn *sio_doipmod_get_doipconn_from_conn(struct sio_socket *sock)
{
    union sio_socket_opt opt = { 0 };
    sio_socket_getopt(sock, SIO_SOCK_PRIVATE, &opt);

    struct sio_doip_conn *rconn = opt.private;
    return rconn;
}

static inline
int sio_doipmod_doip_find_hdr(struct sio_doip_conn *dcon)
{
    int overmust = dcon->array.last - dcon->array.pos;
    do{
        SIO_COND_CHECK_RETURN_VAL(overmust < sizeof(struct doip_hdr), -1);

        struct doip_hdr *hdr = (struct doip_hdr *)dcon->array.pos;
        int err = doip_hdr_version_preamble(hdr);
        if (err == DOIP_HDR_ERR_OK) {
            if (overmust >= sizeof(struct doip_hdr)) {
                return 0;
            } else {
                return -1;
            }
        }

        dcon->array.pos += 2;
        overmust = dcon->array.last - dcon->array.pos;
    } while (overmust >= 2);

    return -1;
}

static inline
void sio_doipmod_doip_stream_parser(struct sio_doip_conn *dcon)
{
    int total = dcon->array.end - dcon->array.buf;
    
    while (1) {
        int ret = sio_doipmod_doip_find_hdr(dcon);
        SIO_COND_CHECK_BREAK(ret != 0);

        struct doip_hdr *hdr = (struct doip_hdr *)dcon->array.pos;
        unsigned int contlen = ntohl(hdr->length);
        int size = sizeof(struct doip_hdr) + contlen;
        if (size > total) {
            sio_doip_dconn_array_realloc(dcon, size * 5 / 4);
            break;
        } else if (size > dcon->array.end - dcon->array.pos) {
            sio_doip_dconn_array_moveused(dcon);
            break;
        }

        struct sio_doip_msg msg = {dcon->array.pos, size};

        int err = doip_hdr_version_valid(hdr);
        SIO_COND_CHECK_CALLOPS_CONTINUE(err != DOIP_HDR_ERR_OK,
            SIO_LOGE("doip tcpdata invalid version, err: %d\n", err),
            dcon->array.pos += size);

        err = doip_hdr_payload_valid(hdr);
        SIO_COND_CHECK_CALLOPS_CONTINUE(err != DOIP_HDR_ERR_OK,
            SIO_LOGE("doip tcpdata invalid payload, err: %d\n", err),
            dcon->array.pos += size);

        sio_doip_handler(dcon->sock, &dcon->contbl, &msg);
        dcon->array.pos += size;
    }

    sio_doip_dconn_array_moveused(dcon);
}

static inline
int sio_doipmod_socket_readable(struct sio_socket *sock)
{
    struct sio_doip_conn *dcon = sio_doipmod_get_doipconn_from_conn(sock);

    int free = dcon->array.end - dcon->array.last;
    int len = sio_socket_read(sock, dcon->array.last, free);
    SIO_COND_CHECK_RETURN_VAL(len <= 0, 0);
    dcon->array.last += len;

    sio_doipmod_doip_stream_parser(dcon);

    return 0;
}

static inline
int sio_doipmod_socket_writeable(struct sio_socket *sock)
{
    return 0;
}

static inline
int sio_doipmod_socket_closeable(struct sio_socket *sock)
{
    return 0;
}

static inline
int sio_doip_init()
{
    sio_doip_node_init(SIO_DOIP_NODETYPE_ENTITY);
    sio_doip_node_setla(1024);
    sio_doip_node_setgid("123456");

    struct sio_socket_addr addr = {"127.0.0.1", 13400};
    sio_doip_discover_init(&addr);

    return 0;
}

sio_submod_t sio_doipmod_create(void)
{
    struct sio_doipmod *doip = malloc(sizeof(struct sio_doipmod));
    SIO_COND_CHECK_RETURN_VAL(!doip, NULL);

    memset(doip, 0, sizeof(struct sio_doipmod));

    doip->maxconn = SIO_DOIP_CONN_MAX;

    sio_list_init(&doip->conhead);

    sio_doip_init();

    return doip;
}

int sio_doipmod_newconn(sio_submod_t mod, struct sio_server *server)
{
    struct sio_socket *sock = sio_socket_create(SIO_SOCK_TCP, NULL);
    union sio_socket_opt opt = {
        .ops.readable = sio_doipmod_socket_readable,
        .ops.writeable = sio_doipmod_socket_writeable,
        .ops.closeable = sio_doipmod_socket_closeable
    };
    sio_socket_setopt(sock, SIO_SOCK_OPS, &opt);

    opt.nonblock = 1;
    sio_socket_setopt(sock, SIO_SOCK_NONBLOCK, &opt);

    int ret = sio_server_accept(server, sock);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        sio_socket_destory(sock));

    struct sio_doip_conn *dcon = sio_doipmod_doipconn_create();
    dcon->mod = mod;
    dcon->sock = sock;

    sio_doip_conhead_add(mod, dcon);

    opt.private = dcon;
    sio_socket_setopt(sock, SIO_SOCK_PRIVATE, &opt);

    ret = sio_server_socket_mplex(server, sock);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        sio_socket_destory(sock));

    return 0;
}

int sio_doipmod_setlocate(sio_submod_t mod, const struct sio_location *locations, int size)
{
    return 0;
}

int sio_doipmod_destory(sio_submod_t mod)
{
    SIO_COND_CHECK_RETURN_VAL(!mod, -1);
    // struct sio_doipmod *doip = (struct sio_doipmod *)mod;
    return 0;
}