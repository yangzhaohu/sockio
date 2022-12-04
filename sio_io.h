#ifndef SIO_IO_H_
#define SIO_IO_H_

struct sio_io_ops
{
    int (*read_cb)(void *owner, const char *data, int len);
    int (*write_cb)(void *owner, const char *data, int len);
};

#endif