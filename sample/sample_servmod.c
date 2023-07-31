#include <stdio.h>
#include "sio_servmod.h"

int main()
{
    struct sio_servmod *servmod = sio_servmod_create(SIO_SUBMOD_HTTP);

    struct sio_location locat[] = {
        {
            .type = SIO_LOCATE_HTTP_MOUDLE,
            .loname = "httpmod1",
            .regex = "^/custom",
            .route = "http_custom_mod1"
        },
        {
            .type = SIO_LOCATE_HTTP_DIRECT,
            .loname = "urlloca1",
            .regex = "^/$",
            .route = "/workspaces/sockio/root/"
        },
        {
            .type = SIO_LOCATE_HTTP_DIRECT,
            .loname = "urlloca2",
            .regex = "^/html$",
            .route = "/workspaces/sockio/root/"
        },
        {
            .type = SIO_LOCATE_HTTP_DIRECT,
            .loname = "urlloca3",
            .regex = "^/html/.*\\.html$",
            .route = "/workspaces/sockio/root/"
        },
        {
            .type = SIO_LOCATE_HTTP_LOCATE,
            .loname = "urlloca4",
            .regex = "^/locate_route$",
            .route = "urlloca3"
        },
        {
            .type = SIO_LOCATE_HTTP_DIRECT,
            .loname = "urlloca5",
            .regex = "^.*/prefix/",
            .route = "/workspaces/sockio/root/"
        }
    };
    sio_servmod_setlocat(servmod, locat, sizeof(locat) / sizeof(struct sio_location));

    union sio_servmod_opt opt = {
        .addr = {"127.0.0.1", 8000}
    };
    sio_servmod_setopt(servmod, SIO_SERVMOD_ADDR, &opt);

    sio_servmod_dowork(servmod);

    getc(stdin);

    sio_servmod_destory(servmod);

    return 0;
}