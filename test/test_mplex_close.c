#include <stdio.h>
#include <string.h>
#include "sio_socket.h"
#include "sio_pmplex.h"

int main()
{
    struct sio_pmplex *pmplex = sio_pmplex_create(SIO_MPLEX_SELECT);

    getc(stdin);

    sio_pmplex_destory(pmplex);

    return 0;
}