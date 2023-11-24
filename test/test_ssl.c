#include <stdio.h>
#include <string.h>
#include "sio_socket.h"
#include "sio_log.h"

int main()
{
    struct sio_socket *sock = sio_socket_create(SIO_SOCK_SSL, NULL);
    if (!sock) {
        SIO_LOGE("socket create failed\n");
        return -1;
    }

    struct sio_sockaddr addr = {
        // .addr = "110.242.68.66",
        .addr = "39.156.66.10",
        .port = 443
    };
    int ret = sio_socket_connect(sock, &addr);
    SIO_LOGI("sock connect ret: %d\n", ret);

    getc(stdin);

    return 0;
}
