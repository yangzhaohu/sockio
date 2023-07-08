#include <stdio.h>
#include "moudle/sio_servmod.h"

int main()
{
    struct sio_servmod *servmod = sio_servmod_create(SIO_SERVMOD_HTTP);
    getc(stdin);
    return 0;
}