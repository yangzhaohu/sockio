#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <pthread.h>
#include "sio_mplex.h"
#include "sio_thread.h"

int g_epfd;

struct sio_mplex *g_mp;

int g_server = 0;
int g_exit = 0;

struct fddesri
{
    int fd;
};

void socket_addrinfo_print(int socket)
{
    struct sockaddr_in l_addr = { 0 }, r_addr = { 0 };
    socklen_t alen = sizeof(l_addr);
    getsockname(socket, (struct sockaddr *)&l_addr, &alen);
    getpeername(socket, (struct sockaddr *)&r_addr, &alen);
    SIO_LOGI("\t local %s:%d \t", inet_ntoa(l_addr.sin_addr), l_addr.sin_port);
    SIO_LOGI("\t remot %s:%d \n", inet_ntoa(r_addr.sin_addr), r_addr.sin_port);
}

int server_accept(int server)
{
    struct sockaddr_in cliaddr = { 0 };
    socklen_t alen = sizeof(cliaddr);

    int client = accept(server, (struct sockaddr *)&cliaddr, &alen);
    if (client < 0) {
        return -1;
    }
    SIO_LOGI("client: %d connected, ip: %s, port: %d\n", client, inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
    socket_addrinfo_print(client);

    struct fddesri *fddes = malloc(sizeof(struct fddesri));
    fddes->fd = client;
    struct sio_event event = { 0 };
    event.event |= EPOLLIN;
    event.owner.ptr = fddes;

    if (sio_mplex_ctl(g_mp, EPOLL_CTL_ADD, client, &event)) {
        SIO_LOGI("client epoll ctl failed\n");
        return -1;
    }
    
    // struct epoll_event event = { 0 };
    // event.events |= EPOLLIN;
    // event.data.fd = client;

    // if (epoll_ctl(g_epfd, EPOLL_CTL_ADD, client, &event) == -1) {
    //     SIO_LOGI("client epoll ctl failed\n");
    //     return -1;
    // }

    return 0;
}

void socket_thread(void *args)
{
    char buffer[64] = { 0 };
    while (!g_exit) {
        int i;
        int rc;
        // struct epoll_event event[32] = { 0 };
        // rc = epoll_wait(g_epfd, event, 32, -1);
        struct sio_event event[16];
        rc = sio_mplex_wait(g_mp, event, 16);
        for (i = 0; i < rc; i++) {
            struct fddesri *fddes = event[i].owner.ptr;
            if (event[i].event & EPOLLIN == EPOLLIN) {
                int fd = fddes->fd;
                if (fd == g_server) {
                    server_accept(g_server);
                } else {
                    int len = recv(fd, buffer, 63, 0);
                    if (len <= 0 ) {
                        SIO_LOGI("socket: %d closed\n", fd);
                        socket_addrinfo_print(fd);
                        close(fd);
                    } else {
                        SIO_LOGI("socket: %d", fd);
                        socket_addrinfo_print(fd);
                        buffer[len] = 0;
                        SIO_LOGI("\nrecv: %s\n\n", buffer);
                    }
                }
            }
        }
    }

    return;
}

int main()
{
    // open
    int server = socket(PF_INET, SOCK_STREAM, 0);
    if (server == -1) {
        SIO_LOGI("server socket open failed\n");
        return -1;
    }
    
    // bind
    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8000);
    int rc = inet_pton(AF_INET, "0.0.0.0", &addr.sin_addr);
    if (rc == 0 || rc == -1) {
        SIO_LOGI("ip trans failed\n");
        return -1;
    }
    rc = bind(server, (struct sockaddr *)&addr, sizeof(addr));
    if (rc == -1) {
        SIO_LOGI("server addr binb failed\n");
        return -1;
    }

    // listen
    rc = listen(server, SOMAXCONN);
    if (rc == -1) {
        SIO_LOGI("server listen failed\n");
        return -1;
    }

    SIO_LOGI("server id: %d\n", server);
    g_server = server;

    g_mp = sio_mplex_create(0);
    if (!g_mp) {
        SIO_LOGI("sio_mplex create failed\n");
        return -1;
    }

    struct fddesri *fddes = malloc(sizeof(struct fddesri));
    fddes->fd = server;
    struct sio_event event = { 0 };
    event.event |= EPOLLIN;
    event.owner.ptr = fddes;

    if (sio_mplex_ctl(g_mp, EPOLL_CTL_ADD, server, &event)) {
        SIO_LOGI("sio_mplex ctl failed\n");
        return -1;
    }

    g_epfd = epoll_create(128);
    if (g_epfd < 0) {
        SIO_LOGI("epoll create failed\n");
        close(server);
        return -1;
    }

    // struct epoll_event event = { 0 };
    // event.events |= EPOLLIN;
    // event.data.fd = server;

    // if (epoll_ctl(g_epfd, EPOLL_CTL_ADD, server, &event) == -1) {
    //     SIO_LOGI("epoll ctl failed\n");
    //     close(server);
    //     return -1;
    // }

    pthread_t tid = -1;
    struct sio_thread *sthread = sio_thread_create(socket_thread, NULL);
    if (!sthread) {
        return -1;
    }

    // pthread_create(&tid, NULL, socket_thread, NULL);

    SIO_LOGI("enter exit\n");
    getc(stdin);
    g_exit = 1;
    close(server);
    pthread_join(tid, NULL);
    
    return 0;
}
