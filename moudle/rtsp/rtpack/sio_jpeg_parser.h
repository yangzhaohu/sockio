#ifndef SIO_JPEG_PARSER_H_
#define SIO_JPEG_PARSER_H_

#include <string.h>

enum sio_jpeg_mark
{
    SIO_JPEG_MARK_START = 0xFF,
    SIO_JPEG_MARK_SOI   = 0xD8,
    SIO_JPEG_MARK_JFIF  = 0xE0,
    SIO_JPEG_MARK_CMT   = 0xFE,
    SIO_JPEG_MARK_DQT   = 0xDB,
    SIO_JPEG_MARK_SOF   = 0xC0,
    SIO_JPEG_MARK_DHT   = 0xC4,
    SIO_JPEG_MARK_SOS   = 0xDA,
    SIO_JPEG_MARK_EOI   = 0xD9,
    SIO_JPEG_MARK_DRI   = 0xDD
};

struct test_jpeg_meta_dqt
{
    unsigned short len;
    unsigned char num:4;
    unsigned char precision:4;
};

struct sio_jpeg_meta_sof
{
    unsigned short len;
    unsigned char precision;
    unsigned short width;
    unsigned short height;
    unsigned char count;
} __attribute__((packed));

struct sio_jpeg_meta
{
    const unsigned char *start;
    const unsigned char *end;
    struct test_jpeg_meta_dqt dqt;
    const unsigned char *qtbl1;
    const unsigned char *qtbl2;

    struct sio_jpeg_meta_sof sof;

    unsigned short dri;
};

#ifdef __cplusplus
extern "C" {
#endif

static inline
int sio_jpeg_parser(struct sio_jpeg_meta *meta, const unsigned char *data, int len)
{
    for (int i = 0; i < len - 1; i++) {
        if (data[i] == SIO_JPEG_MARK_START && data[i + 1] == SIO_JPEG_MARK_SOI) {
            meta->start = data + i;
            break;
        }
    }

    if (meta->start == NULL) {
        return -1;
    }

    for (int i = len - 1; i > 0; i--) {
        if (data[i - 1] == SIO_JPEG_MARK_START && data[i] == SIO_JPEG_MARK_EOI) {
            meta->end = data + i;
            break;
        }
    }

    if (meta->end == NULL) {
        return -1;
    }

    for (int i = meta->start - data; i < meta->end - meta->start; i++) {
        if (meta->start[i] != SIO_JPEG_MARK_START) {
            continue;
        }
        switch (meta->start[i + 1]) {
        case SIO_JPEG_MARK_JFIF:
        case SIO_JPEG_MARK_DHT:
        {
            unsigned short headersize = ((unsigned short)meta->start[i + 2]) << 8 | meta->start[i + 3];
            i += headersize + 1;
            break;
        }
        case SIO_JPEG_MARK_SOF:
        {
            unsigned short headersize = ((unsigned short)meta->start[i + 2]) << 8 | meta->start[i + 3];
            struct sio_jpeg_meta_sof *sof = &meta->sof;
            const unsigned char *width = meta->start + i + 5;
            sof->width = ((unsigned short)width[0]) << 8 | width[1];
            const unsigned char *height = width + 2;
            sof->height = ((unsigned short)height[0]) << 8 | height[1];

            i += headersize + 1;
            break;
        }
        case SIO_JPEG_MARK_SOS:
        {
            unsigned short headersize = ((unsigned short)meta->start[i + 2]) << 8 | meta->start[i + 3];
            i += headersize + 1;
            break;
        }

        case SIO_JPEG_MARK_DQT:
        {
            unsigned short headersize = ((unsigned short)meta->start[i + 2]) << 8 | meta->start[i + 3];
            if (meta->qtbl1 == NULL) {
                meta->qtbl1 = meta->start + i + 5;
            } else {
                meta->qtbl2 = meta->start + i + 5;
            }
            i += headersize + 1;
            break;
        }

        case SIO_JPEG_MARK_DRI:
        {
            unsigned short headersize = ((unsigned short)meta->start[i + 2]) << 8 | meta->start[i + 3];
            if (headersize == 4) {
                const unsigned char *dri = meta->start + i + 4;
                meta->dri = ((unsigned short)dri[0]) << 8 | dri[1];
                i += headersize + 1;
            }
            break;
        }
        
        default:
            break;
        }
    }

    return 0;
}

#ifdef __cplusplus
}
#endif

#endif