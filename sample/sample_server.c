#include <stdio.h>
#include <string.h>
#include "sio_server.h"

int main()
{
    struct sio_server *serv = sio_server_create(SIO_SOCK_TCP, NULL);

    struct sio_socket_addr addr = {"127.0.0.1", 8000};
    sio_server_listen(serv, &addr);

    getc(stdin);

    return 0;
}