#ifndef SIO_ERRNO_H_
#define SIO_ERRNO_H_

enum sio_errno
{
    SIO_ERRNO_OK = 0,
    SIO_ERRNO_FAILED = -1,
    SIO_ERRNO_AGAIN = -2,
    SIO_ERRNO_IGNORE = -3
};

#endif