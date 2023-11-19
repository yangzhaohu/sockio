#include <stdio.h>
#include "sio_common.h"
#include "sio_socket.h"
#include "sio_permplex.h"
#include "sio_list.h"
#include "sio_timer.h"
#include "moudle/doip/doip_hdr.h"
#include "sio_log.h"

struct test_doipmsg{
    char *data;
    unsigned int only; // only copy byte length
    unsigned int len; // msg length
    unsigned int size; // buffer size
};

struct test_doip_entity
{
    struct sio_list_head entry;
    char gid[7];
    char eid[7];
    char vin[18];
    unsigned short ela;
    unsigned char far;
    unsigned char ess;

    struct sio_sockaddr addr;
    struct sio_socket *tcpsock;

    struct sio_timer *alivetimer;

    struct test_doipmsg dmsg;
};

struct test_equip
{
    unsigned short la;
    struct sio_permplex *pmplex;
    struct sio_socket *udpsock;
    struct sio_list_head head;

    //
    unsigned long int ela; // current ctrl entity la
};

struct test_equip g_tequip = { 0 };

#define TEST_EQUIP                  (&g_tequip)
#define TEST_EQUIP_LA               (TEST_EQUIP->la)
#define TEST_EQUIP_PERMPLEX         (TEST_EQUIP->pmplex)
#define TEST_EQUIP_UDP              (TEST_EQUIP->udpsock)
#define TEST_EQUIP_ENTITY_HEAD      &(TEST_EQUIP->head)

#define TEST_EQUIP_ENTITY_LA        (TEST_EQUIP->ela)

static inline struct test_doip_entity *test_equip_entity_find(int fd, unsigned short la);
static inline int test_equip_add_entity(struct test_doip_entity *entity);
static void test_equip_doip_route_req(struct sio_socket *tcpsock);

static inline void *test_equip_alivetimer_routine(void *arg);

static inline
int test_equip_entity_alivetimer_init(struct test_doip_entity *entity)
{
    if (entity->alivetimer == NULL) {
        entity->alivetimer = sio_timer_create(test_equip_alivetimer_routine, entity);
        SIO_COND_CHECK_CALLOPS_RETURN_VAL(entity->alivetimer == NULL, -1,
            SIO_LOGE("entity alivetimer create failed\n"));

        sio_timer_start(entity->alivetimer, 1000 * 1000, 1000 * 1000);
    }

    return 0;
}

static inline
int test_equip_entity_update(struct doip_hdr *hdr, unsigned int len, struct sio_sockaddr *peer)
{
    struct doip_anno *anno = (struct doip_anno *)((char *)hdr + sizeof(struct doip_hdr));
    unsigned short la = ntohs(anno->la);
    struct test_doip_entity *entity = test_equip_entity_find(0, la);
    if (entity == NULL) {
        entity = malloc(sizeof(struct test_doip_entity));
        SIO_COND_CHECK_RETURN_VAL(entity == NULL, -1);
        memset(entity, 0, sizeof(struct test_doip_entity));
    }

    memcpy(entity->gid, anno->gid, 6);
    memcpy(entity->eid, anno->eid, 6);
    memcpy(entity->vin, anno->vin, 17);
    memcpy(&entity->addr, peer, sizeof(struct sio_sockaddr));
    entity->ela = la;
    entity->far = anno->far;
    entity->ess = anno->ss;

    test_equip_add_entity(entity);

    return 0;
}

static inline
unsigned short test_equip_udp_packet_handler(char *buf, unsigned int len, struct sio_sockaddr *peer)
{
    struct doip_hdr *hdr = (struct doip_hdr *)buf;
    unsigned short type = ntohs(hdr->type);
    switch (type) {
    case DOIP_TYPE_ANNO:
        test_equip_entity_update(hdr, len, peer);
        break;
    
    default:
        break;
    }

    return 0;
}

static inline
int test_equip_route_ack_process(char *buf, unsigned int len)
{
    struct doip_route_ack *route = (struct doip_route_ack *)(buf + sizeof(struct doip_hdr));
    unsigned short sa = ntohs(route->sa);
    unsigned short la = ntohs(route->la);
    unsigned char code = route->code;
    SIO_LOGI("route active sa: %d route la: %d\n", sa, la);
    SIO_LOGI("route active code: %d\n", code);

    return code;
}

static inline
int test_equip_status_ack_process(char *buf, unsigned int len)
{
    struct doip_status_ack *status = (struct doip_status_ack *)(buf + sizeof(struct doip_hdr));
    SIO_LOGI("entity status: \n"
        "type:    %d\n"
        "maxconn: %d\n"
        "curconn: %d\n"
        "maxproc: %d\n", status->type, status->maxconn, status->curconn, ntohl(status->maxprocess));

    return 0;
}

static inline
int test_equip_tcp_packet_handler(struct test_doip_entity *entity, char *buf, unsigned int len)
{
    struct doip_hdr *hdr = (struct doip_hdr *)buf;
    unsigned short type = ntohs(hdr->type);
    switch (type) {
    case DOIP_TYPE_ROUTE_ACK:
    {
        int code = test_equip_route_ack_process(buf, len);
        if (code) {
            test_equip_entity_alivetimer_init(entity);
        }
        break;
    }
    
    case DOIP_TYPE_STATUS_ACK:
        test_equip_status_ack_process(buf, len);
        break;
    
    default:
        break;
    }

    return 0;
}

static int test_equip_socket_readable(struct sio_socket *sock)
{
    union sio_sockopt opt = { 0 };
    sio_socket_getopt(sock, SIO_SOCK_PRIVATE, &opt);
    struct test_doip_entity *entity = opt.private;
    struct test_doipmsg *dmsg = &entity->dmsg;

    char buf[1024] = { 0 };
    int rect = sio_socket_read(sock, buf, 1024);
    SIO_COND_CHECK_RETURN_VAL(rect <= 0, rect);

    int read = 0;
    do {
        if (dmsg->len == 0) {
            unsigned int min = rect > sizeof(struct doip_hdr) ? sizeof(struct doip_hdr) : rect;
            memcpy(dmsg->data + dmsg->only, buf, min);
            dmsg->only += min;
            read += min;
            if (dmsg->only < sizeof(struct doip_hdr)) {
                return rect;
            }

            struct doip_hdr *dihdr = (struct doip_hdr *)dmsg->data;
            unsigned int contlen = ntohl(dihdr->length);

            unsigned int size = sizeof(struct doip_hdr) + contlen;
            dmsg->len = size;

            if (size > dmsg->size) {
                void *data = malloc(size);
                memcpy(data, dmsg->data, dmsg->only);
                free(dmsg->data);

                dmsg->data = data;
                dmsg->size = size;
            }
        }

        int overmust = dmsg->len - dmsg->only;
        int min = (rect - read) > overmust ? overmust : (rect - read);
        memcpy(dmsg->data + dmsg->only, buf + read, min);
        dmsg->only += min;
        read += min;

        if (dmsg->only < dmsg->len) {
            break;
        }

        // handler
        test_equip_tcp_packet_handler(entity, dmsg->data, dmsg->len);
        // reset msgone

        dmsg->only = 0;
        dmsg->len = 0;
    } while (read < rect);

    return 0;
}

static int test_equip_socket_closeable(struct sio_socket *sock)
{
    SIO_LOGI("socket close\n");
    sio_socket_destory(sock);
    return 0;
}

static int test_equip_socket_readable_from(struct sio_socket *sock)
{
    char buf[256] = { 0 };
    struct sio_sockaddr peer = { 0 };
    int ret = sio_socket_readfrom(sock, buf, 256, &peer);
    if (ret <= 0) {
        return ret;
    }

    SIO_LOGI("recv form: %s:%d\n\n", peer.addr, peer.port);
    test_equip_udp_packet_handler(buf, 256, &peer);

    return 0;
}

static inline
int test_equip_connect_entity(int fd, unsigned short la)
{
    struct test_doip_entity *entity = test_equip_entity_find(fd,la);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(entity == NULL, -1,
        SIO_LOGE("not found entity %d\n", la));

    SIO_COND_CHECK_CALLOPS_RETURN_VAL(entity->tcpsock, -1,
        SIO_LOGE("entity %d already connect\n", la));

    struct test_doipmsg *dmsg = &entity->dmsg;
    dmsg->data = malloc(2048);
    memset(dmsg->data, 0, 2048);
    dmsg->len = dmsg->only = 0;
    dmsg->size = 2048;

    entity->tcpsock = sio_socket_create(SIO_SOCK_TCP, NULL);

    union sio_sockopt opt = { 0 };
    opt.ops.readable = test_equip_socket_readable;
    opt.ops.closeable = test_equip_socket_closeable;
    int ret = sio_socket_setopt(entity->tcpsock, SIO_SOCK_OPS, &opt);

    opt.nonblock = 1;
    ret = sio_socket_setopt(entity->tcpsock, SIO_SOCK_NONBLOCK, &opt);

    opt.private = entity;
    ret = sio_socket_setopt(entity->tcpsock, SIO_SOCK_PRIVATE, &opt);

    struct sio_mplex *mplex = sio_permplex_mplex_ref(TEST_EQUIP_PERMPLEX);
    opt.mplex = mplex;
    sio_socket_setopt(entity->tcpsock, SIO_SOCK_MPLEX, &opt);

    sio_socket_connect(entity->tcpsock, &entity->addr);

    sio_socket_mplex(entity->tcpsock, SIO_EV_OPT_ADD, SIO_EVENTS_IN);

    test_equip_doip_route_req(entity->tcpsock);

    return 0;
}

static void test_equip_doip_identify_req()
{
    char buf[128] = { 0 };
    doip_gene_hdr_identify(buf);

    struct sio_sockaddr peer = {"0.0.0.0", 13400};
    sio_socket_writeto(TEST_EQUIP_UDP, buf, sizeof(struct doip_hdr), &peer);
}

static void test_equip_doip_route_req(struct sio_socket *tcpsock)
{
    char buf[128] = { 0 };
    doip_gene_hdr_route(buf);

    struct doip_route *route = (struct doip_route *)(buf + sizeof(struct doip_hdr));
    route->sa = htons(TEST_EQUIP_LA);

    SIO_LOGI("route active request...\n");
    sio_socket_write(tcpsock, buf, sizeof(struct doip_hdr) + sizeof(struct doip_route));
}

static void test_equip_doip_status_req(struct sio_socket *tcpsock)
{
    char buf[128] = { 0 };
    doip_gene_hdr_status(buf);
    SIO_LOGI("entity status request...\n");
    sio_socket_write(tcpsock, buf, sizeof(struct doip_hdr));
}

static void test_equip_doip_alive_req(struct sio_socket *tcpsock)
{
    char buf[128] = { 0 };
    doip_gene_hdr_alive(buf);
    sio_socket_write(tcpsock, buf, sizeof(struct doip_hdr));
}

static inline
void *test_equip_alivetimer_routine(void *arg)
{
    struct test_doip_entity *entity = (struct test_doip_entity *)arg;
    test_equip_doip_alive_req(entity->tcpsock);
    return NULL;
}

static inline
int test_equip_configure_params()
{
    TEST_EQUIP->la = 999;

    return 0;
}

static inline
int test_equip_permplex_init()
{
    TEST_EQUIP->pmplex = sio_permplex_create(SIO_MPLEX_EPOLL);
    SIO_COND_CHECK_RETURN_VAL(TEST_EQUIP_PERMPLEX == NULL, -1);

    return 0;
}

static inline
int test_equip_udpsock_init()
{
    TEST_EQUIP->udpsock = sio_socket_create(SIO_SOCK_UDP, NULL);
    SIO_COND_CHECK_RETURN_VAL(TEST_EQUIP_UDP == NULL, -1);

    union sio_sockopt opt = { 0 };
    opt.ops.readable = test_equip_socket_readable_from;
    opt.ops.closeable = test_equip_socket_closeable;
    int ret = sio_socket_setopt(TEST_EQUIP_UDP, SIO_SOCK_OPS, &opt);

    opt.nonblock = 1;
    ret = sio_socket_setopt(TEST_EQUIP_UDP, SIO_SOCK_NONBLOCK, &opt);

    struct sio_sockaddr addr = {"127.0.0.1", 13401};
    sio_socket_bind(TEST_EQUIP_UDP, &addr);

    struct sio_mplex *mplex = sio_permplex_mplex_ref(TEST_EQUIP_PERMPLEX);
    opt.mplex = mplex;
    sio_socket_setopt(TEST_EQUIP_UDP, SIO_SOCK_MPLEX, &opt);

    sio_socket_mplex(TEST_EQUIP_UDP, SIO_EV_OPT_ADD, SIO_EVENTS_IN);

    return 0;
}

static inline
int test_equip_init()
{
    int ret = test_equip_configure_params();

    ret = test_equip_permplex_init();
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        SIO_LOGE("test permplex init failed\n"));

    ret = test_equip_udpsock_init();
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        SIO_LOGE("test udpsock init failed\n"));

    sio_list_init(TEST_EQUIP_ENTITY_HEAD);
}

static inline
struct test_doip_entity *test_equip_entity_find(int fd, unsigned short la)
{
    struct test_doip_entity *node = NULL;
    struct sio_list_head *pos;
    sio_list_foreach(pos, TEST_EQUIP_ENTITY_HEAD) {
        node = (struct test_doip_entity *)pos;
        if (node->ela == la) {
            return node;
        }
    }

    return NULL;
}

static inline
int test_equip_add_entity(struct test_doip_entity *entity)
{
    struct test_doip_entity *node = NULL;
    node = test_equip_entity_find(0, entity->ela);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(node == NULL, 0,
        sio_list_add(&entity->entry, TEST_EQUIP_ENTITY_HEAD));

    // update
    memcpy(node->gid, entity->gid, 6);
    memcpy(node->eid, entity->eid, 6);
    memcpy(node->vin, entity->vin, 17);
    node->far = entity->far;
    node->ess = entity->ess;

    return 0;
}

static inline
int test_equip_del_entity(int fd, unsigned short la)
{
    struct test_doip_entity *node = NULL;
    node = test_equip_entity_find(0, la);
    SIO_COND_CHECK_RETURN_VAL(node == NULL, 0);

    sio_list_del(&node->entry);

    return 0;
}

static inline
int test_equip_ls_entity()
{
    struct test_doip_entity *node = NULL;
    struct sio_list_head *pos;
    sio_list_foreach(pos, TEST_EQUIP_ENTITY_HEAD) {
        node = (struct test_doip_entity *)pos;
        unsigned long int id = node->ela;
        SIO_LOGE("entity %lu:\n"
        "   ela: %d\n"
        "   eip: %s, port: %d\n"
        "   gid: %s\n"
        "   eid: %s\n"
        "   vin: %s\n"
        "   far: %d\n"
        "   ess: %d\n", id, node->ela, node->addr.addr, node->addr.port,
        node->gid, node->eid, node->vin, node->far, node->ess);
    }
}

static inline
int test_equip_ctrllink_ela()
{
    if (TEST_EQUIP_ENTITY_LA == 0) {
        SIO_LOGI("enter entity la:");
        scanf("%lu", &TEST_EQUIP_ENTITY_LA);
    }
    return TEST_EQUIP_ENTITY_LA;
}

int main()
{
    int ret = test_equip_init();
    SIO_COND_CHECK_RETURN_VAL(ret == -1, -1);
    
    while (1) {
        char c = getc(stdin);
        switch (c) {
        case 'e':
            test_equip_doip_identify_req();
            break;
        case 'l':
            test_equip_ls_entity();
            break;
        case 'i':
            SIO_LOGI("enter new entity la: ");
            scanf("%lu", &TEST_EQUIP_ENTITY_LA);
            break;
        case 'c':
        {
            unsigned long int ela = 0;
            ela = test_equip_ctrllink_ela();
            test_equip_connect_entity(0, ela & 0xFFFF);
            break;
        }
        case 'r':
        {
            unsigned long int ela = 0;
            ela = test_equip_ctrllink_ela();
            struct test_doip_entity *entity;
            entity = test_equip_entity_find(0, ela & 0xFFFF);
            test_equip_doip_route_req(entity->tcpsock);
            break;
        }
        case 's':
        {
            unsigned long int ela = 0;
            ela = test_equip_ctrllink_ela();
            struct test_doip_entity *entity;
            entity = test_equip_entity_find(0, ela & 0xFFFF);
            test_equip_doip_status_req(entity->tcpsock);
            break;
        }
        case 'q':
            return 0;
        
        default:
            break;
        }
    }

    return 0;
}