#include "sio_doip_handler.h"
#include <stdio.h>
#include "doip_hdr.h"
#include "sio_common.h"
#include "sio_doip_route.h"
#include "sio_socket.h"
#include "sio_timer.h"
#include "sio_doip_node.h"
#include "sio_doip_err.h"
#include "sio_log.h"

static inline
unsigned char sio_doip_err_transto_route_code(enum sio_doip_err err)
{
    unsigned char code = 0;
    switch (err) {
    case SIO_DOIP_ERR_ROUTE_REJECT_UNKOWNSA:
        code = DOIP_ROUTE_ACK_CODE_UNKNOWN;
        break;
    case SIO_DOIP_ERR_ROUTE_REJECT_REPEAT:
        code = DOIP_ROUTE_ACK_CODE_REPEAT;
        break;
    case SIO_DOIP_ERR_ROUTE_REJECT_DIFFSA:
        code = DOIP_ROUTE_ACK_CODE_DIFFSA;
        break;
    case SIO_DOIP_ERR_ROUTE_OK:
        code = DOIP_ROUTE_ACK_CODE_OK;
        break;
    
    default:
        break;
    }

    return code;
}

static inline
int sio_doip_route_reply(struct sio_socket *sock, unsigned short sa, unsigned char code)
{
    const int size = sizeof(struct doip_hdr) + sizeof(struct doip_route_ack);
    char buf[size];
    doip_gene_hdr_route_ack(buf);

    struct doip_route_ack *rack = (struct doip_route_ack *)(buf + sizeof(struct doip_hdr));
    rack->sa = htons(sa);
    rack->la = htons(sio_doip_node_getla());
    rack->code = code;
    rack->resv1 = 0;
    rack->resv2 = 0;

    sio_socket_write(sock, buf, size);
    return 0;
}

static inline
int sio_doip_status_reply(struct sio_socket *sock, struct sio_doip_contbl *contbl)
{
    const int size = sizeof(struct doip_hdr) + sizeof(struct doip_status_ack);
    char buf[size];
    doip_gene_hdr_status_ack(buf);
    struct doip_status_ack *status = (struct doip_status_ack *)(buf + sizeof(struct doip_hdr));
    status->type = 0x00;
    status->maxconn = sio_doip_node_getcap_maxconn();
    status->curconn = sio_doip_node_getcap_curconn();
    status->maxprocess = htonl(sio_doip_node_getcap_process());

    sio_socket_write(sock, buf, size);
    return 0;
}

static inline
void sio_doip_reset_general_timer(struct sio_doip_contbl *contbl)
{
    if (1) {
        sio_timer_start(contbl->genetimer, SIO_GENERAL_TIMEROUT_SEC * 1000 * 1000, 0);
    }
}

int sio_doip_handler(struct sio_socket *sock, struct sio_doip_contbl *contbl, struct sio_doip_msg *msg)
{
    struct doip_hdr *hdr = (struct doip_hdr *)msg->data;
    unsigned short type = ntohs(hdr->type);
    unsigned int length = ntohl(hdr->length);
    printf("doiphdr:\n"
        "\t ver: %02x\n"
        "\t iver: %02x\n"
        "\t type: %04d\n"
        "\t len: %4d\n", hdr->ver, hdr->iver, type, length);

    sio_doip_reset_general_timer(contbl);

    switch (type) {
    case DOIP_TYPE_ROUTE:
    {
        struct doip_route *route = (struct doip_route *)((char *)hdr + sizeof(struct doip_hdr));
        int err = sio_doip_route_process(contbl, route);
        unsigned char code = sio_doip_err_transto_route_code(err);
        unsigned short sa = ntohs(route->sa);
        sio_doip_route_reply(sock, sa, code);

        sio_timer_stop(contbl->inittimer);
        sio_timer_start(contbl->genetimer, 300 * 1000 * 1000, 0);
        break;
    }
    case DOIP_TYPE_ALIVE:
        break;
    case DOIP_TYPE_STATUS:
        sio_doip_status_reply(sock, contbl);
        break;
    case DOIP_TYPE_POWER:
        break;
    case DOIP_TYPE_DIAGNOSTIC:
        break;
    }
    return 0;
}

int sio_doip_data_version_check(struct doip_hdr *hdr)
{
    return hdr->ver + hdr->iver == 0 ? 0 : -1;
}