#include "sio_httpmod_html.h"
#include <string.h>

static inline
int sio_http_head_append(char *buf, const char *field)
{
    int len = strlen(field);
    memcpy(buf, field, len);

    return len;
}

static inline
int sio_httpmod_notfound(sio_conn_t conn, char *headbuf, int size)
{
    const char *body = "<!DOCTYPE html>\r\n"
                    "<html>\r\n"
                    "<head><title>404 Not Found</title></head>\r\n"
                    "<body>\r\n"
                    "<center><h1>404 Not Found</h1></center>\r\n"
                    "<hr><center>SOCKIO</center>\r\n"
                    "</body>\r\n"
                    "</html>\r\n";

    int l = sio_http_head_append(headbuf, "HTTP/1.1 404\r\n");
    l += sio_http_head_append(headbuf + l, "Connection: close\r\n");
    l += sio_http_head_append(headbuf + l, "Content-Type: text/html\r\n");
    char cl[64] = { 0 };
    int len = strlen(body);
    sprintf(cl, "Content-Length: %d\r\n", len);
    l += sio_http_head_append(headbuf + l, cl);
    l += sio_http_head_append(headbuf + l, "\r\n");

    sio_conn_write(conn, headbuf, l);
    sio_conn_write(conn, body, len);
    sio_conn_close(conn);

    return 0;
}

int sio_httpmod_html_response(sio_conn_t conn, char *headbuf, int size, const char *file)
{
    FILE *fp = fopen(file, "r");
    if (fp == NULL) {
        SIO_LOGE("%s not found\n", file);
        sio_httpmod_notfound(conn, headbuf, size);
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    int l = sio_http_head_append(headbuf, "HTTP/1.1 200\r\n");
    l += sio_http_head_append(headbuf + l, "Connection: keep-alive\r\n");
    l += sio_http_head_append(headbuf + l, "Content-Type: text/html\r\n");
    char cl[64] = { 0 };
    sprintf(cl, "Content-Length: %ld\r\n", len);
    l += sio_http_head_append(headbuf + l, cl);
    l += sio_http_head_append(headbuf + l, "\r\n");

    sio_conn_write(conn, headbuf, l);

    while (1) {
        char tmp[1024] = { 0 };
        l = fread(tmp, 1, 1024, fp);
        if (l <= 0) {
            break;
        }
        sio_conn_write(conn, tmp, l);
    }

    fclose(fp);

    return 0;
}