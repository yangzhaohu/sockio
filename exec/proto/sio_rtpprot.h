#ifndef SIO_RTPPROT_H_
#define SIO_RTPPROT_H_

#include <arpa/inet.h>
#include "sio_endian.h"

#define SIO_RTP_OVER_TCP_HEADBYTE 4

#define SIO_RTP_PAYLOAD_TYPE_JPEG   26
#define SIO_RTP_PAYLOAD_TYPE_H264   96

struct sio_rtphdr
{
#if SIO_BYTE_ORDER == SIO_BIG_ENDIAN
    unsigned int version:2;
    unsigned int padding:1;
    unsigned int ext:1;
    unsigned int cc:4;

    unsigned int mark:1;
    unsigned int type:7;
#elif SIO_BYTE_ORDER == SIO_LITTLE_ENDIAN
    unsigned int cc:4;
    unsigned int ext:1;
    unsigned int padding:1;
    unsigned int version:2;

    unsigned int type:7;
    unsigned int mark:1;
#else
# error	"Unknown endian"
#endif
    unsigned short seq;
    unsigned int timestamp;
    unsigned int ssrc;
} __attribute__((packed));

#ifdef __cplusplus
extern "C" {
#endif

static inline
void sio_rtp_header_set_version(struct sio_rtphdr *head, unsigned char version)
{
    head->version = version & 0x03;
}

static inline
void sio_rtp_header_set_padding(struct sio_rtphdr *head, unsigned char padding)
{
    head->padding = padding & 0x01;
}

static inline
void sio_rtp_header_set_extension(struct sio_rtphdr *head, unsigned char ext)
{
    head->ext = ext & 0x01;
}

static inline
void sio_rtp_header_set_cc(struct sio_rtphdr *head, unsigned char cc)
{
    head->cc = cc & 0x0F;
}

static inline
void sio_rtp_header_set_mark(struct sio_rtphdr *head, unsigned char mark)
{
    head->mark = mark & 0x01;
}

static inline
void sio_rtp_header_set_type(struct sio_rtphdr *head, unsigned char type)
{
    head->type = type & 0x7F;
}

static inline
void sio_rtp_header_set_seq(struct sio_rtphdr *head, unsigned short seq)
{
    head->seq = htons(seq);
}

static inline
void sio_rtp_header_set_timestamp(struct sio_rtphdr *head, unsigned int timestamp)
{
    head->timestamp = htonl(timestamp);
}

static inline
void sio_rtp_header_set_ssrc(struct sio_rtphdr *head, unsigned int ssrc)
{
    head->ssrc = htonl(ssrc);
}

#ifdef __cplusplus
}
#endif


#endif