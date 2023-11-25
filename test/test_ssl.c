#include <stdio.h>
#include <string.h>
#include "sio_socket.h"
#include "sio_log.h"

int main()
{
    struct sio_socket *sock = sio_socket_create2(SIO_SOCK_SSL, NULL);
    if (!sock) {
        SIO_LOGE("socket create failed\n");
        return -1;
    }

    // union sio_sockopt opt = { 0 };
    // opt.nonblock = 1;
    // sio_socket_setopt(sock, SIO_SOCK_NONBLOCK, &opt);

    struct sio_sockaddr addr = {
        // .addr = "110.242.68.66",
        .addr = "39.156.66.10",
        .port = 443
    };
    int ret = sio_socket_connect(sock, &addr);
    SIO_LOGI("sock connect ret: %d\n", ret);

    const char *request = "GET / HTTP/1.0\r\n"
                          "Connection: Keep-Alive\r\n"
                          "Host: 127.0.0.1:8000\r\n"
                          "User-Agent: ApacheBench/2.3\r\n"
                          "Accept: */*\r\n\r\n";
    ret = sio_socket_write(sock, request, strlen(request));
    SIO_LOGE("write len: %d, real: %d\n", ret, strlen(request));

    while (1) {
        char buf[512] = { 0 };
        int ret = sio_socket_read(sock, buf, 511);
        if (ret > 0) {
            SIO_LOGI("%s", buf);
        }
        char c = getc(stdin);
        if (c == 'q') {
            break;
        }
    }

    getc(stdin);

    return 0;
}
