#ifndef SIO_AVFRAME_H_
#define SIO_AVFRAME_H_

struct sio_avinfo
{
    unsigned char encode;
    unsigned char fps;
    unsigned short width;
    unsigned short height;
};

struct sio_avframe
{
    unsigned int timestamp;
    unsigned char *data;
    unsigned int length;
};

#endif