#include <stdio.h>
#include <string.h>
#include "sio_socket.h"
#include "sio_permplex.h"

int main()
{
    struct sio_permplex *pmplex = sio_permplex_create(SIO_MPLEX_SELECT);

    getc(stdin);

    sio_permplex_destory(pmplex);

    return 0;
}