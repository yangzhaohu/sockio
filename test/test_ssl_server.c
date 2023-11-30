#include <stdio.h>
#include <string.h>
#include "sio_socket.h"
#include "sio_permplex.h"
#include "sio_log.h"

int main()
{
    struct sio_socket *serv = sio_socket_create(SIO_SOCK_SSL, NULL);

    union sio_sockopt opt = { 0 };
    opt.data = "../cert/ca.crt";
    sio_socket_setopt(serv, SIO_SOCK_SSL_CACERT, &opt);
    opt.data = "../cert/server.crt";
    sio_socket_setopt(serv, SIO_SOCK_SSL_USERCERT, &opt);
    opt.data = "../cert/server.key";
    sio_socket_setopt(serv, SIO_SOCK_SSL_USERKEY, &opt);

    struct sio_sockaddr addr = {
        .addr = "127.0.0.1",
        .port = 8000
    };
    int ret = sio_socket_listen(serv, &addr);
    SIO_LOGI("sock listen ret: %d\n", ret);

    while(1) {
        if (sio_socket_accept_has_pend(serv) != 0) {
            continue;
        }

        struct sio_socket *sock = sio_socket_create(SIO_SOCK_SSL, NULL);
        int ret = sio_socket_accept(serv, sock);
        if (ret != 0) {
            SIO_LOGI("accept failed\n");
            continue;
        } else {
            char buf[512] = { 0 };
            ret = sio_socket_read(sock, buf, 511);
            if (ret > 0) {
                SIO_LOGI("%s\n", buf);
            }
            sio_socket_write(sock, "hello, client\n", strlen("hello, client\n"));
        }

        // sio_socket_shutdown(sock, SIO_SOCK_SHUTRDWR);
        sio_socket_destory(sock);
    }

    return 0;
}