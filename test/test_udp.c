#include <stdio.h>
#include <string.h>
#include "sio_socket.h"
#include "sio_log.h"

int main()
{
    struct sio_socket *sock = sio_socket_create(SIO_SOCK_UDP, NULL);
    struct sio_sockaddr addr = { "127.0.0.1", 8090};
    sio_socket_bind(sock, &addr);

    struct sio_socket *sock2 = sio_socket_create(SIO_SOCK_UDP, NULL);
    struct sio_sockaddr addr2 = { "127.0.0.1", 8091};
    sio_socket_bind(sock2, &addr2);

    int l = sio_socket_writeto(sock, "hello sock2", strlen("hello sock2"), &addr2);
    SIO_LOGI("send: %s\n    to: %s:%d\n", "hello sock2", addr2.addr, addr2.port);

    char buf[256] = { 0 };
    l = sio_socket_readfrom(sock2, buf, 255, &addr);
    SIO_LOGI("recv: %s\n    form: %s:%d\n", buf, addr.addr, addr.port);

    getc(stdin);

    sio_socket_destory(sock);
    sio_socket_destory(sock2);

    return 0;
}