#include "doip_hdr.h"

#define DOIP_CASE(if) \
    case if:

#define DOIP_VARSION_SUPPORT_CASE_MAP       \
    DOIP_CASE(DOIP_VERSION_1)               \
    DOIP_CASE(DOIP_VERSION_2)

#define DOIP_PAYLOAD_SUPPORT_CASE_MAP       \
    DOIP_CASE(DOIP_TYPE_NACK)               \
    DOIP_CASE(DOIP_TYPE_IDENTTITY)          \
    DOIP_CASE(DOIP_TYPE_IDENTTITY_EID)      \
    DOIP_CASE(DOIP_TYPE_IDENTTITY_VIN)      \
    DOIP_CASE(DOIP_TYPE_ANNO)               \
    DOIP_CASE(DOIP_TYPE_ROUTE)              \
    DOIP_CASE(DOIP_TYPE_ROUTE_ACK)          \
    DOIP_CASE(DOIP_TYPE_ALIVE)              \
    DOIP_CASE(DOIP_TYPE_ALIVE_ACK)          \
    DOIP_CASE(DOIP_TYPE_STATUS)             \
    DOIP_CASE(DOIP_TYPE_STATUS_ACK)         \
    DOIP_CASE(DOIP_TYPE_POWER)              \
    DOIP_CASE(DOIP_TYPE_POWER_ACK)          \
    DOIP_CASE(DOIP_TYPE_DIAGNOSTIC)         \
    DOIP_CASE(DOIP_TYPE_DIAGNOSTIC_ACK)     \
    DOIP_CASE(DOIP_TYPE_DIAGNOSTIC_NACK)

void *doip_message_alloc(unsigned int len)
{
    len += sizeof(struct doip_hdr);
    char *buf = (char *)malloc(len);
    if (buf == NULL) {
        return NULL;
    }

    return buf + sizeof(struct doip_hdr);
}

void doip_message_free(void *buf)
{
    char *base = (char *)buf;
    base -= sizeof(struct doip_hdr);
    free(base);
}

static inline
int doip_check_version_support(unsigned char ver)
{
    int support = -1;
    switch (ver) {
    DOIP_VARSION_SUPPORT_CASE_MAP
        support = 0;
        break;

    default:
        break;
    }

    return support;
}

static inline
int doip_check_payload_support(unsigned short payload)
{
    int support = -1;
    switch (payload) {
    DOIP_PAYLOAD_SUPPORT_CASE_MAP
        support = 0;
        break;

    default:
        break;
    }

    return support;
}

int doip_hdr_version_preamble(struct doip_hdr *hdr)
{
    int err = DOIP_HDR_ERR_OK;
    if ((hdr->ver + hdr->iver) != 0XFF) {
        err = DOIP_HDR_ERR_UNEQUAL;
    }

    return err;
}

int doip_hdr_version_valid(struct doip_hdr *hdr)
{
    int err = DOIP_HDR_ERR_OK;
    if (doip_check_version_support(hdr->ver) == -1) {
        err = DOIP_HDR_ERR_UNVERSION;
    }

    return err;
}

int doip_hdr_payload_valid(struct doip_hdr *hdr)
{
    int err = DOIP_HDR_ERR_OK;
    if (doip_check_payload_support(ntohs(hdr->type)) == -1) {
        err = DOIP_HDR_ERR_UNPAYLOAD;
    }

    return err;
}

void doip_gene_hdr_nack(char *data)
{
    struct doip_hdr *hdr = (struct doip_hdr *)data;
    hdr->ver = 0x02;
    hdr->iver = ~(0x02);
    hdr->type = DOIP_TYPE_NACK;
    hdr->length = htonl(sizeof(struct doip_nack));
}

void doip_gene_hdr_identify(char *data)
{
    struct doip_hdr *hdr = (struct doip_hdr *)data;
    hdr->ver = 0x02;
    hdr->iver = ~(0x02);
    hdr->type = htons(DOIP_TYPE_IDENTTITY);
    hdr->length = 0x00;
}

void doip_gene_hdr_identify_eid(char *data)
{
    struct doip_hdr *hdr = (struct doip_hdr *)data;
    hdr->ver = 0x02;
    hdr->iver = ~(0x02);
    hdr->type = htons(DOIP_TYPE_IDENTTITY_EID);
    hdr->length = htonl(sizeof(struct doip_identify_eid));
}

void doip_gene_hdr_identify_vin(char *data)
{
    struct doip_hdr *hdr = (struct doip_hdr *)data;
    hdr->ver = 0x02;
    hdr->iver = ~(0x02);
    hdr->type = htons(DOIP_TYPE_IDENTTITY_VIN);
    hdr->length = htonl(sizeof(struct doip_identify_vin));
}

void doip_gene_hdr_anno(char *data)
{
    struct doip_hdr *hdr = (struct doip_hdr *)data;
    hdr->ver = 0x02;
    hdr->iver = ~(0x02);
    hdr->type = htons(DOIP_TYPE_ANNO);
    hdr->length = htonl(sizeof(struct doip_anno));
}

void doip_gene_hdr_route(char *data)
{
    struct doip_hdr *hdr = (struct doip_hdr *)data;
    hdr->ver = 0x02;
    hdr->iver = ~(0x02);
    hdr->type = htons(DOIP_TYPE_ROUTE);
    hdr->length = htonl(sizeof(struct doip_route));
}

void doip_gene_hdr_route_ack(char *data)
{
    struct doip_hdr *hdr = (struct doip_hdr *)data;
    hdr->ver = 0x02;
    hdr->iver = ~(0x02);
    hdr->type = htons(DOIP_TYPE_ROUTE_ACK);
    hdr->length = htonl(sizeof(struct doip_route_ack));
}

void doip_gene_hdr_alive(char *data)
{
    struct doip_hdr *hdr = (struct doip_hdr *)data;
    hdr->ver = 0x02;
    hdr->iver = ~(0x02);
    hdr->type = htons(DOIP_TYPE_ALIVE);
    hdr->length = 0x00;
}

void doip_gene_hdr_alive_ack(char *data)
{
    struct doip_hdr *hdr = (struct doip_hdr *)data;
    hdr->ver = 0x02;
    hdr->iver = ~(0x02);
    hdr->type = htons(DOIP_TYPE_ALIVE_ACK);
    hdr->length = htonl(sizeof(struct doip_alive_ack));
}

void doip_gene_hdr_status(char *data)
{
    struct doip_hdr *hdr = (struct doip_hdr *)data;
    hdr->ver = 0x02;
    hdr->iver = ~(0x02);
    hdr->type = htons(DOIP_TYPE_STATUS);
    hdr->length = 0x00;
}

void doip_gene_hdr_status_ack(char *data)
{
    struct doip_hdr *hdr = (struct doip_hdr *)data;
    hdr->ver = 0x02;
    hdr->iver = ~(0x02);
    hdr->type = htons(DOIP_TYPE_STATUS_ACK);
    hdr->length = htonl(sizeof(struct doip_status_ack));
}

void doip_gene_hdr_power(char *data)
{
    struct doip_hdr *hdr = (struct doip_hdr *)data;
    hdr->ver = 0x02;
    hdr->iver = ~(0x02);
    hdr->type = htons(DOIP_TYPE_POWER);
    hdr->length = 0x00;
}

void doip_gene_hdr_power_ack(char *data)
{
    struct doip_hdr *hdr = (struct doip_hdr *)data;
    hdr->ver = 0x02;
    hdr->iver = ~(0x02);
    hdr->type = htons(DOIP_TYPE_POWER_ACK);
    hdr->length = htonl(sizeof(struct doip_power_ack));
}

void doip_gene_hdr_diagnostic(char *data, unsigned int len)
{
    struct doip_hdr *hdr = (struct doip_hdr *)data;
    hdr->ver = 0x02;
    hdr->iver = ~(0x02);
    hdr->type = htons(DOIP_TYPE_DIAGNOSTIC);
    hdr->length = htonl(len);
}

void doip_gene_hdr_diagnostic_ack_ack(char *data, unsigned int len)
{
    struct doip_hdr *hdr = (struct doip_hdr *)data;
    hdr->ver = 0x02;
    hdr->iver = ~(0x02);
    hdr->type = htons(DOIP_TYPE_DIAGNOSTIC_ACK);
    hdr->length = htonl(len);
}

void doip_gene_hdr_diagnostic_nack_ack(char *data, unsigned int len)
{
    struct doip_hdr *hdr = (struct doip_hdr *)data;
    hdr->ver = 0x02;
    hdr->iver = ~(0x02);
    hdr->type = htons(DOIP_TYPE_DIAGNOSTIC_NACK);
    hdr->length = htonl(len);
}