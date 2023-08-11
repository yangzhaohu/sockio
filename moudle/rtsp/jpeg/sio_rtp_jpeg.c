#include "sio_rtp_jpeg.h"
#include <stdlib.h>
#include <string.h>
#include "sio_common.h"
#include "proto/sio_rtpprot.h"
#include "proto/sio_jpegprot.h"
#include "sio_jpeg_parser.h"

struct sio_rtp_jpeg
{
    unsigned int size;
    unsigned char *buffer;
    unsigned char *rtp;
    unsigned int seq;
};

#define sio_rtp_packet_size(jpeg) jpeg->size - SIO_RTP_OVER_TCP_HEADBYTE

struct sio_rtp_jpeg *sio_rtp_jpeg_create(unsigned int size)
{
    struct sio_rtp_jpeg *jpeg = malloc(sizeof(struct sio_rtp_jpeg));
    SIO_COND_CHECK_RETURN_VAL(!jpeg, NULL);

    memset(jpeg, 0, sizeof(struct sio_rtp_jpeg));

    size = size + SIO_RTP_OVER_TCP_HEADBYTE + sizeof(struct sio_rtphdr);
    jpeg->buffer = malloc(size);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(jpeg->buffer == NULL, NULL,
        free(jpeg));
    
    jpeg->size = size;
    jpeg->rtp = jpeg->buffer + SIO_RTP_OVER_TCP_HEADBYTE;

    return jpeg;
}

static inline
void sio_rtp_jpeg_rtphdr_zero(struct sio_rtphdr *rtphdr)
{
    sio_rtp_header_set_version(rtphdr, 2);
    sio_rtp_header_set_padding(rtphdr, 0);
    sio_rtp_header_set_extension(rtphdr, 0);
    sio_rtp_header_set_cc(rtphdr, 0);
    sio_rtp_header_set_type(rtphdr, SIO_RTP_PAYLOAD_TYPE_JPEG);
    sio_rtp_header_set_mark(rtphdr, 0);
    sio_rtp_header_set_ssrc(rtphdr, 0);
}

static inline
void sio_rtp_jpeg_jpeghdr_zero(struct sio_jpeghdr *jpeghdr, struct sio_jpeg_meta *meta,
    unsigned int offset)
{
    jpeghdr->tspec = 0;
    // offset
    char *offptr = (char *)jpeghdr + 1;
    offptr[0] = (offset & 0x00FF0000) >> 16;
    offptr[1] = (offset & 0x0000FF00) >> 8;
    offptr[2] = (offset & 0x000000FF);
    jpeghdr->type = 1 | (meta->dri != 0 ? 0x40 : 0);
    jpeghdr->quant = 255;
    jpeghdr->width = meta->sof.width / 8;
    jpeghdr->height = meta->sof.height / 8;
}

static inline
unsigned int sio_rtp_jpeg_jpeghdr_dqt_zero(struct sio_jpeghdr_dqt *dqt, struct sio_jpeg_meta *meta)
{
    dqt->mbz = 0;
    dqt->precision = 0;
    dqt->len = htons(128);

    unsigned int offset = sizeof(struct sio_jpeghdr_dqt);

    memcpy((char *)dqt + offset, meta->qtbl1, 64);
    offset += 64;
    memcpy((char *)dqt + offset, meta->qtbl2, 64);
    offset += 64;

    return offset;
}

int sio_rtp_jpeg_process(struct sio_rtp_jpeg *jpeg, const unsigned char *data, unsigned int len,
    void (*payload)(const unsigned char *data, unsigned int len))
{
    SIO_COND_CHECK_RETURN_VAL(!jpeg || !payload, -1);

    struct sio_jpeg_meta meta = { 0 };
    int ret = sio_jpeg_parser(&meta, data, len);
    SIO_COND_CHECK_RETURN_VAL(ret != 0, -1);

    struct sio_rtphdr *rtphdr = (struct sio_rtphdr *)jpeg->rtp;
    sio_rtp_jpeg_rtphdr_zero(rtphdr);

    unsigned int timestamp = 0;
    unsigned int offset = 0;
    while (offset < len) {
        unsigned char *ptr = jpeg->rtp;

        sio_rtp_header_set_seq(rtphdr, jpeg->seq);
        sio_rtp_header_set_timestamp(rtphdr, timestamp);

        jpeg->seq++;
        ptr += sizeof(struct sio_rtphdr);

        struct sio_jpeghdr *jpeghdr = (struct sio_jpeghdr *)ptr;
        sio_rtp_jpeg_jpeghdr_zero(jpeghdr, &meta, offset);
        ptr += sizeof(struct sio_jpeghdr);

        if (meta.dri != 0) {
        }

        if (jpeghdr->quant > 128 && offset == 0) {
            struct sio_jpeghdr_dqt *dqt = (struct sio_jpeghdr_dqt *)(ptr);
            unsigned int off = sio_rtp_jpeg_jpeghdr_dqt_zero(dqt, &meta);
            ptr += off;
        }

        unsigned int l = sio_rtp_packet_size(jpeg) - (ptr - jpeg->rtp);
        if (l >= len - offset) {
            l = len - offset;
            sio_rtp_header_set_mark(rtphdr, 1);
        }

        memcpy(ptr, data + offset, l);
        payload(jpeg->rtp, l + (ptr - jpeg->rtp));

        offset += l;
    }

    return 0;
}

int sio_rtp_jpeg_destory(struct sio_rtp_jpeg *jpeg)
{
    SIO_COND_CHECK_RETURN_VAL(!jpeg, -1);
    free(jpeg->buffer);
    free(jpeg);

    return 0;
}