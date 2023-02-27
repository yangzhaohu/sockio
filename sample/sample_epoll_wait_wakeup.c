#include <stdio.h>
#include <string.h>
#include "sio_mplex_thread.h"

int main()
{
    struct sio_mplex_thread *mpthr = sio_mplex_thread_create(SIO_MPLEX_EPOLL);
    if (mpthr == NULL) {
        return -1;
    }

    getc(stdin);

    sio_mplex_thread_destory(mpthr);
    
    getc(stdin);

    return 0;
}