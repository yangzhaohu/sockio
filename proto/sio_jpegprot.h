#ifndef SIO_JPEGPROT_H_
#define SIO_JPEGPROT_H_

#include "sio_endian.h"

struct sio_jpeghdr
{
    unsigned int tspec:8;
    unsigned int offset:24;
    unsigned char type;
    unsigned char quant;
    unsigned char width;
    unsigned char height;
} __attribute__((packed));

struct sio_jpeghdr_dqt
{
    unsigned char mbz;
    unsigned char precision;
    unsigned short len;
} __attribute__((packed));

struct sio_jpeghdr_rst
{
    unsigned short dri;
    unsigned int f:1;
    unsigned int l:1;
    unsigned int count:14;
};

#endif