#include <stdio.h>
#include "sio_servmod.h"

int main()
{
    struct sio_servmod *servmod = sio_servmod_create(SIO_SUBMOD_HTTP);

    struct sio_locate locat[] = {
        {
            .type = SIO_HTTP_LOCA_ROOT,
            .loname = "rootpath",
            .route = "/workspaces/sockio/root/"
        },
        {
            .type = SIO_HTTP_LOCA_INDEX,
            .loname = "homepage",
            .regex = "/",
            .route = "index.html"
        },
        {
            .type = SIO_HTTP_LOCA_MOD,
            .loname = "httpmod1",
            .regex = "/testmod",
            .route = "http_custom_mod1"
        },
        {
            .type = SIO_HTTP_LOCA_VAL,
            .loname = "urlloca1",
            .regex = "/html",
            .route = "/workspaces/sockio/root/html/"
        }
    };
    sio_servmod_setlocat(servmod, locat, sizeof(locat) / sizeof(struct sio_locate));

    union sio_servmod_opt opt = {
        .addr = {"127.0.0.1", 8000}
    };
    sio_servmod_setopt(servmod, SIO_SERVMOD_ADDR, &opt);

    sio_servmod_dowork(servmod);

    getc(stdin);

    return 0;
}