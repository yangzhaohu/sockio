#include <stdio.h>
#include <string.h>
#include "sio_permplex.h"

int main()
{
    struct sio_permplex *mpthr = sio_permplex_create(SIO_MPLEX_EPOLL);
    if (mpthr == NULL) {
        return -1;
    }

    getc(stdin);

    sio_permplex_destory(mpthr);
    
    getc(stdin);

    return 0;
}