#include <stdio.h>
#include "sio_service.h"

int main()
{
    struct sio_service *servmod = sio_service_create(SIO_SUBMOD_RTSP);

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
        },
        {
            .type = SIO_LOCATE_RTSP_VOD,
            .loname = "urlloca6",
            .regex = "^.*/vod/",
            .route = "/workspaces/sockio/root/"
        },
        {
            .type = SIO_LOCATE_RTSP_LIVE,
            .loname = "urlloca7",
            .regex = "^.*/live/",
            .route = "/workspaces/sockio/root/"
        }
    };

    sio_service_setlocat(servmod, locat, sizeof(locat) / sizeof(struct sio_location));

    union sio_service_opt opt = {
        .addr = {"127.0.0.1", 8000}
    };
    sio_service_setopt(servmod, SIO_SERVICE_ADDR, &opt);

    sio_service_dowork(servmod);

    getc(stdin);

    sio_service_destory(servmod);

    return 0;
}