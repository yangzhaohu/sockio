#include <stdio.h>
#include <string.h>
#include "sio_servers.h"

int main()
{
    struct sio_servers *servs = sio_servers_create2(SIO_SOCK_TCP, 16);

    struct sio_socket_addr addr = {"127.0.0.1", 8000};
    sio_servers_listen(servs, &addr);

    getc(stdin);

    sio_servers_destory(servs);

    getc(stdin);

    return 0;
}