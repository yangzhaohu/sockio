#include <stdio.h>
#include "moudle/sio_servmod.h"

int main()
{
    struct sio_servmod *servmod = sio_servmod_create(SIO_SERVMOD_HTTP);

    union sio_servmod_opt opt = {
        .addr = {"127.0.0.1", 8000}
    };
    sio_servmod_setopt(servmod, SIO_SERVMOD_ADDR, &opt);

    sio_servmod_dowork(servmod);

    getc(stdin);

    return 0;
}