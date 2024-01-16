#ifndef DOIP_HDR_H_
#define DOIP_HDR_H_

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define DOIP_VERSION_1          0x01
#define DOIP_VERSION_2          0x02

#define DOIP_TYPE_NACK              0x0000
#define DOIP_TYPE_IDENTTITY         0x0001
#define DOIP_TYPE_IDENTTITY_EID     0x0002
#define DOIP_TYPE_IDENTTITY_VIN     0x0003
#define DOIP_TYPE_ANNO              0x0004
#define DOIP_TYPE_ROUTE             0x0005
#define DOIP_TYPE_ROUTE_ACK         0x0006
#define DOIP_TYPE_ALIVE             0x0007
#define DOIP_TYPE_ALIVE_ACK         0x0008
#define DOIP_TYPE_STATUS            0x4001
#define DOIP_TYPE_STATUS_ACK        0x4002
#define DOIP_TYPE_POWER             0x4003
#define DOIP_TYPE_POWER_ACK         0x4004
#define DOIP_TYPE_DIAGNOSTIC        0x8001
#define DOIP_TYPE_DIAGNOSTIC_ACK    0x8002
#define DOIP_TYPE_DIAGNOSTIC_NACK   0x8003

#define DOIP_ROUTE_ACK_CODE_UNKNOWN         0x00
#define DOIP_ROUTE_ACK_CODE_REPEAT          0x01
#define DOIP_ROUTE_ACK_CODE_DIFFSA          0x02
#define DOIP_ROUTE_ACK_CODE_DIFFCON         0x03
#define DOIP_ROUTE_ACK_CODE_NOAUTH          0x04
#define DOIP_ROUTE_ACK_CODE_NOCONFIRM       0x05
#define DOIP_ROUTE_ACK_CODE_UNSUPPORT       0x06
#define DOIP_ROUTE_ACK_CODE_OK              0x10
#define DOIP_ROUTE_ACK_CODE_WAITCONFIRM     0x11

struct doip_hdr
{
    unsigned char ver;
    unsigned char iver;
    unsigned short type;
    unsigned int length;
    unsigned char data[0];
} __attribute__((packed));

struct doip_nack
{
    unsigned char code;
} __attribute__((packed));

struct doip_identify_eid
{
    unsigned char eid[6];
} __attribute__((packed));

struct doip_identify_vin
{
    unsigned char vin[17];
} __attribute__((packed));

struct doip_anno
{
    unsigned char vin[17];
    unsigned short la;      // doip entity logical address
    unsigned char eid[6];
    unsigned char gid[6];
    unsigned char far;
    unsigned char ss;
} __attribute__((packed));

// __attribute__((aligned(n))) n字节对齐

struct doip_route
{
    unsigned short sa;      // test equipment logical address
    unsigned char type;
    unsigned int resv1;
    unsigned int resv2;
} __attribute__((packed));

struct doip_route_ack
{
    unsigned short sa;      // test equipment logical address
    unsigned short la;      // doip entity logical address
    unsigned char code;
    unsigned int resv1;
    unsigned int resv2;
} __attribute__((packed));

struct doip_alive_ack
{
    unsigned short sa;      // test equipment logical address
} __attribute__((packed));

struct doip_status_ack
{
    unsigned char type;
    unsigned char maxconn;
    unsigned char curconn;
    unsigned int maxprocess;
} __attribute__((packed));

struct doip_power_ack
{
    unsigned char mode;
} __attribute__((packed));

struct doip_diagnostic
{
    unsigned short sa;      // test equipment logical address
    unsigned short da;      // ecu entity logical address
    unsigned char data[0];  // ISO 14229-1 diagnostic request data
} __attribute__((packed));

struct doip_diagnostic_ack_ack
{
    unsigned short sa;      // ecu entity logical address
    unsigned short da;      // test equipment logical address
    unsigned char ack;
    unsigned char data[0];
} __attribute__((packed));

struct doip_diagnostic_nack_ack
{
    unsigned short sa;      // ecu entity logical address
    unsigned short da;      // test equipment logical address
    unsigned char nack;
    unsigned char data[0];
} __attribute__((packed));

enum doip_hdr_err
{
    DOIP_HDR_ERR_OK,
    DOIP_HDR_ERR_UNEQUAL,
    DOIP_HDR_ERR_UNVERSION,
    DOIP_HDR_ERR_UNPAYLOAD
};

void *doip_message_alloc(unsigned int len);

void doip_message_free(void *buf);

int doip_hdr_version_preamble(struct doip_hdr *hdr);

int doip_hdr_version_valid(struct doip_hdr *hdr);

int doip_hdr_payload_valid(struct doip_hdr *hdr);

void doip_gene_hdr_nack(char *data);

void doip_gene_hdr_identify(char *data);

void doip_gene_hdr_identify_eid(char *data);

void doip_gene_hdr_identify_vin(char *data);

void doip_gene_hdr_anno(char *data);

void doip_gene_hdr_route(char *data);

void doip_gene_hdr_route_ack(char *data);

void doip_gene_hdr_alive(char *data);

void doip_gene_hdr_alive_ack(char *data);

void doip_gene_hdr_status(char *data);

void doip_gene_hdr_status_ack(char *data);

void doip_gene_hdr_power(char *data);

void doip_gene_hdr_power_ack(char *data);

void doip_gene_hdr_diagnostic(char *data, unsigned int len);

void doip_gene_hdr_diagnostic_ack_ack(char *data, unsigned int len);

void doip_gene_hdr_diagnostic_nack_ack(char *data, unsigned int len);

#endif