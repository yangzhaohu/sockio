#include <stdio.h>
#include <string.h>
#include "sio_pmplex.h"

int main()
{
    struct sio_pmplex *mpthr = sio_pmplex_create(SIO_MPLEX_EPOLL);
    if (mpthr == NULL) {
        return -1;
    }

    getc(stdin);

    sio_pmplex_destory(mpthr);
    
    getc(stdin);

    return 0;
}