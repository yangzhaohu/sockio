#include <stdio.h>
#include <string.h>
#include "sio_socket.h"
#include "sio_mplex_thread.h"

int main()
{
    struct sio_socket *sock = sio_socket_create(SIO_SOCK_UDP, NULL);
    struct sio_socket_addr addr = { "127.0.0.1", 8090};
    sio_socket_bind(sock, &addr);

    struct sio_socket *sock2 = sio_socket_create(SIO_SOCK_UDP, NULL);
    struct sio_socket_addr addr2 = { "127.0.0.1", 8091};
    sio_socket_bind(sock2, &addr2);

    int l = sio_socket_writeto(sock, "hello sock2", strlen("hello sock2"), &addr2);
    printf("send: %s\n    to: %s:%d\n", "hello sock2", addr2.addr, addr2.port);

    char buf[256] = { 0 };
    l = sio_socket_readfrom(sock2, buf, 255, &addr);
    printf("recv: %s\n    form: %s:%d\n", buf, addr.addr, addr.port);

    getc(stdin);

    return 0;
}