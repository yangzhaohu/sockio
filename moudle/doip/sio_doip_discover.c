#include "sio_doip_discover.h"
#include <stdlib.h>
#include <string.h>
#include "sio_common.h"
#include "sio_socket.h"
#include "sio_pmplex.h"
#include "sio_doip_node.h"
#include "doip_hdr.h"
#include "sio_log.h"

struct sio_doip_disc
{
    struct sio_socket *udpsock;
    struct sio_pmplex *pmplex;
};

struct sio_doip_disc g_disc;

#define SIO_DOIP_DISC g_disc

static inline
int sio_doip_anno_reply(struct sio_socket *sock, struct sio_sockaddr *peer)
{
    const int size = sizeof(struct doip_hdr) + sizeof(struct doip_anno);
    char buf[size];
    memset(buf, 0, size);
    doip_gene_hdr_anno(buf);
    struct doip_anno *anno = (struct doip_anno *)(buf + sizeof(struct doip_hdr));
    memcpy(anno->vin, sio_doip_node_getvin(), SIO_VIN_LENGTH);
    memcpy(anno->eid, sio_doip_node_geteid(), SIO_ID_LENGTH);
    memcpy(anno->gid, sio_doip_node_getgid(), SIO_ID_LENGTH);
    anno->la = htons(sio_doip_node_getla());
    anno->far = 0x00;
    anno->ss = 0x01;

    return sio_socket_writeto(sock, buf, size, peer);
}

static inline
int sio_doip_discover_handler(struct sio_socket *sock, struct sio_sockaddr *peer, char *buf, int len)
{
    struct doip_hdr *hdr = (struct doip_hdr *)buf;
    int err = doip_hdr_version_preamble(hdr);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(err != DOIP_HDR_ERR_OK, -1,
        SIO_LOGE("doip_discover_handler invalid version preamble, err: %d\n", err));
    err = doip_hdr_version_valid(hdr);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(err != DOIP_HDR_ERR_OK, -1,
        SIO_LOGE("doip_discover_handler invalid version, err: %d\n", err));
    err = doip_hdr_payload_valid(hdr);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(err != DOIP_HDR_ERR_OK, -1,
        SIO_LOGE("doip_discover_handler invalid payload, err: %d\n", err));
    // int contlen = ntohl(hdr->length);
    // int packlen = sizeof(struct doip_hdr) + contlen;
    
    unsigned short type = ntohs(hdr->type);
    unsigned int length = ntohs(hdr->length);
    SIO_LOGI("disvocer doiphdr:\n"
        "\t ver: %02x\n"
        "\t iver: %02x\n"
        "\t type: %04d\n"
        "\t len: %4d\n", hdr->ver, hdr->iver, type, length);

    switch (type) {
    case DOIP_TYPE_IDENTTITY:
        sio_doip_anno_reply(sock, peer);
        break;
    case DOIP_TYPE_IDENTTITY_EID:
    {
        struct doip_identify_eid *eid = (struct doip_identify_eid *)(buf + sizeof(struct doip_hdr));
        if (memcmp(eid->eid, sio_doip_node_geteid(), SIO_ID_LENGTH) == 0) {
            sio_doip_anno_reply(sock, peer);
        }
        break;
    }
    case DOIP_TYPE_IDENTTITY_VIN:
    {
        struct doip_identify_vin *vin = (struct doip_identify_vin *)(buf + sizeof(struct doip_hdr));
        if (memcmp(vin->vin, sio_doip_node_getvin(), SIO_VIN_LENGTH) == 0) {
            sio_doip_anno_reply(sock, peer);
        }
        break;
    }
    default:
        break;
    }

    return 0;
}

static inline
int sio_doip_discover_readable(struct sio_socket *sock)
{
    char buf[512] = { 0 };
    struct sio_sockaddr peer = { 0 };
    int len = sio_socket_readfrom(sock, buf, 512, &peer);
    SIO_COND_CHECK_RETURN_VAL(len <= 0, -1);

    SIO_LOGI("recv form: %s:%d\n\n", peer.addr, peer.port);

    sio_doip_discover_handler(sock, &peer, buf, len);

    return 0;
}

static inline
int sio_doip_discover_writeable(struct sio_socket *sock)
{
    return 0;
}

static inline
int sio_doip_discover_closeable(struct sio_socket *sock)
{
    SIO_LOGI("socket close\n");
    sio_socket_destory(sock);
    return 0;
}

int sio_doip_discover_sock_mplex(struct sio_socket *sock, struct sio_pmplex *pmplex)
{
    union sio_sockopt opt = { 0 };
    opt.ops.readable = sio_doip_discover_readable;
    opt.ops.closeable = sio_doip_discover_closeable;
    sio_socket_setopt(sock, SIO_SOCK_OPS, &opt);

    opt.nonblock = 1;
    sio_socket_setopt(sock, SIO_SOCK_NONBLOCK, &opt);

    struct sio_mplex *mplex = sio_pmplex_mplex_ref(pmplex);
    opt.mplex = mplex;
    sio_socket_setopt(sock, SIO_SOCK_MPLEX, &opt);

    sio_socket_mplex(sock, SIO_EV_OPT_ADD, SIO_EVENTS_IN);

    return 0;
}

int sio_doip_discover_init(struct sio_sockaddr *addr)
{
    struct sio_socket *sock = sio_socket_create(SIO_SOCK_UDP, NULL);
    SIO_COND_CHECK_RETURN_VAL(!sock, -1);

    sio_socket_bind(sock, addr);

    struct sio_pmplex *pmplex = sio_pmplex_create(SIO_MPLEX_SELECT);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!pmplex, -1,
        sio_socket_destory(sock));

    sio_doip_discover_sock_mplex(sock, pmplex);

    SIO_DOIP_DISC.udpsock = sock;
    SIO_DOIP_DISC.pmplex = pmplex;

    return 0;
}