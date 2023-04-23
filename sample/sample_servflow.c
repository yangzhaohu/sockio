#include <stdio.h>
#include <string.h>
#include "sio_servflow.h"

static char *g_resp =
            "HTTP/1.1 200 OK\r\n"
            "Connection: keep-alive\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 125\r\n\r\n"
            "<html>"
            "<head><title>Hello</title></head>"
            "<body>"
            "<center><h1>Hello, Client</h1></center>"
            "<hr><center>SOCKIO</center>"
            "</body>"
            "</html>";

int servflow_flow_new(struct sio_sockflow *flow)
{
    return 0;
}

int servflow_flow_data(struct sio_sockflow *flow, const char *data, int len)
{
    sio_sockflow_write(flow, g_resp, strlen(g_resp));
    return 0;
}

int main()
{
    struct sio_servflow *servflow = sio_servflow_create2(SIO_SERVFLOW_TCP, 2, 4);

    union sio_servflow_opt opt = {
        .ops.flow_new = servflow_flow_new,
        .ops.flow_data = servflow_flow_data
    };
    sio_servflow_setopt(servflow, SIO_SERVFLOW_OPS, &opt);

    struct sio_servflow_addr addr = {"127.0.0.1", 8000};
    sio_servflow_listen(servflow, &addr);

    getc(stdin);

    sio_servflow_destory(servflow);

    getc(stdin);

    return 0;
}