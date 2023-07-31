#include "sio_httpmod.h"
#include <string.h>
#include <stdlib.h>
#include "sio_common.h"
#include "sio_def.h"
#include "sio_list.h"
#include "moudle/sio_locate.h"
#include "moudle/sio_locatmod.h"
#include "proto/sio_httpprot.h"
#include "sio_httpmod_html.h"
#include "sio_log.h"

struct sio_http_env
{
    struct sio_locate *locate;
    struct sio_locatmod *locatmod;
};

enum sio_httpds_index
{
    SIO_HTTPDS_URL,
    SIO_HTTPDS_KEEPALIVE,
    SIO_HTTPDS_PACKET,
    SIO_HTTPDS_BUTT
};

struct sio_httpds
{
    sio_str_t dsstr;
    int dsint;
};

struct sio_http_buffer
{
    char *buf;
    char *pos;
    char *last;
    char *end;
};

struct sio_http_conn
{
    sio_conn_t conn;
    int matchpos;
    struct sio_location location;
    struct sio_http_buffer buf;
    struct sio_httpprot *httpprot;
    struct sio_httpds ds[SIO_HTTPDS_BUTT];

    unsigned int complete:1; // packet complete
};

static struct sio_http_env g_httpenv = { 0 };

#define sio_macro_string(name) name
#define sio_httpenv_get_field(filed) g_httpenv.sio_macro_string(filed)

int sio_httpmod_name(const char **name)
{
    *name = "sio_http_core_module";
    return 0;
}

int sio_httpmod_type()
{
    return SIO_SUBMOD_HTTP;
}

int sio_httpmod_version(const char **version)
{
    *version = "0.0.1";
    return 0;
}

static char *g_resp =
            "HTTP/1.1 200 OK\r\n"
            "Connection: close\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 125\r\n\r\n"
            "<html>"
            "<head><title>Hello</title></head>"
            "<body>"
            "<center><h1>Hello, Client</h1></center>"
            "<hr><center>SOCKIO</center>"
            "</body>"
            "</html>";

static inline
int sio_httpmod_conn_keep(struct sio_http_conn *httpconn)
{
    struct sio_httpds *ds = httpconn->ds;
    if (ds[SIO_HTTPDS_KEEPALIVE].dsint == 1) {
        return 0;
    }

    return -1;
}

int sio_httpmod_protostat(void *handler, enum sio_httpprot_stat type)
{
    return 0;
}

int sio_httpmod_protofield(void *handler, const sio_str_t *field, const sio_str_t *val)
{
    struct sio_http_conn *httpconn = handler;
    struct sio_httpds *ds = httpconn->ds;

    if (strncmp(field->data, "Connection", strlen("Connection")) == 0) {
        ds[SIO_HTTPDS_KEEPALIVE].dsstr.data = val->data;
        ds[SIO_HTTPDS_KEEPALIVE].dsstr.length = val->length;
        if (strncasecmp(val->data, "Keep-Alive", strlen("Keep-Alive")) == 0) {
            ds[SIO_HTTPDS_KEEPALIVE].dsint = 1;
        }
    }

    return 0;
}

int sio_httpmod_protodata(void *handler, enum sio_httpprot_data type, const sio_str_t *data)
{
    // SIO_LOGE("type: %d, %.*s\n", type, (int)len, data);

    int len = data->length;
    struct sio_http_conn *httpconn = handler;
    struct sio_httpds *ds = httpconn->ds;

    if (type == SIO_HTTPPROTO_DATA_URL) {
        if (len > 4096) {
            sio_conn_write(httpconn->conn, g_resp, strlen(g_resp));
            sio_conn_close(httpconn->conn);
            printf("url too large\n");
            return -1;
        }
        ds[SIO_HTTPDS_URL].dsstr.data = data->data;
        ds[SIO_HTTPDS_URL].dsstr.length = len;

        struct sio_locate *locate = sio_httpenv_get_field(locate);
        struct sio_location location = { 0 };
        int matchpos = sio_locate_match(locate, data->data, len, &location);
        if (matchpos != -1) {
            httpconn->location = location;
            httpconn->matchpos = matchpos;
        }
    }

    return len;
}

static inline
int sio_httpmod_find_base_url_pos(const char *url, int len)
{
    for (len--; len > 0; len--) {
        if (url[len] == '/') {
            break;
        }
    }

    return len + 1;
}

static inline
int sio_httpmod_protoover(void *handler)
{
    struct sio_http_conn *httpconn = handler;
    struct sio_httpds *ds = httpconn->ds;

    httpconn->complete = 1;
    const char *url = ds[SIO_HTTPDS_URL].dsstr.data;
    int len = ds[SIO_HTTPDS_URL].dsstr.length;

    struct sio_location *location = &httpconn->location;
    if (location->type == SIO_LOCATE_HTTP_DIRECT) {
        char name[512] = { 0 };
        int baseurl = sio_httpmod_find_base_url_pos(url, httpconn->matchpos);
        sprintf(name, "%s%.*s", location->route, len - baseurl, url + baseurl);
        printf("name: %s\n", name);
        char head[4096] = { 0 };
        sio_httpmod_html_response(httpconn->conn, head, 4096, name);
        if (sio_httpmod_conn_keep(httpconn) != 0) {
            sio_conn_close(httpconn->conn);
        }
    }

    return 0;
}

static inline
struct sio_http_conn *sio_httpmod_httpconn_create(sio_conn_t conn)
{
    struct sio_http_conn *httpconn = malloc(sizeof(struct sio_http_conn));
    SIO_COND_CHECK_RETURN_VAL(!httpconn, NULL);

    memset(httpconn, 0, sizeof(struct sio_http_conn));

    // string
    struct sio_http_buffer *buf = &httpconn->buf;
    char *mem = malloc(40960);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!mem, NULL,
        free(httpconn));

    memset(mem, 0, 40960);
    buf->buf = mem;
    buf->pos = mem;
    buf->last = mem;
    buf->end = mem + 40960;

    // httpprot
    struct sio_httpprot *httpprot = sio_httpprot_create(SIO_HTTP_REQUEST);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!httpprot, NULL,
        free(mem),
        free(httpconn));
    union sio_httpprot_opt hopt = {
        .private = httpconn
    };
    sio_httpprot_setopt(httpprot, SIO_HTTPPROT_PRIVATE, &hopt);
    hopt.ops.prot_stat = sio_httpmod_protostat;
    hopt.ops.prot_field = sio_httpmod_protofield;
    hopt.ops.prot_data = sio_httpmod_protodata;
    hopt.ops.prot_over = sio_httpmod_protoover;
    sio_httpprot_setopt(httpprot, SIO_HTTPPROT_OPS, &hopt);
    httpconn->httpprot = httpprot;

    // conn with httpconn
    union sio_conn_opt opt = { .private = httpconn };
    sio_conn_setopt(conn, SIO_CONN_PRIVATE, &opt);
    httpconn->conn = conn;

    return httpconn;
}

static inline
void sio_httpmod_httpconn_destory(struct sio_http_conn *httpconn)
{
    sio_httpprot_destory(httpconn->httpprot);
    
    free(httpconn->buf.buf);
    free(httpconn);
}

int sio_httpmod_create(void)
{
    sio_httpenv_get_field(locate) = sio_locate_create();
    sio_httpenv_get_field(locatmod) = sio_locatmod_create();

    return 0;
}

int sio_httpmod_setlocat(const struct sio_location *locations, int size)
{
    struct sio_locate *locate = sio_httpenv_get_field(locate);
    for (int i = 0; i < size; i++) {
        sio_locate_add(locate, &locations[i]);
    }

    return 0;
}

int sio_httpmod_hookmod(const char *modname, struct sio_submod *mode)
{
    return 0;
}

int sio_httpmod_streamconn(sio_conn_t conn)
{
    sio_httpmod_httpconn_create(conn);
    return 0;
}

int sio_httpmod_streamin(sio_conn_t conn, const char *data, int len)
{
    union sio_conn_opt opt = { 0 };
    sio_conn_getopt(conn, SIO_CONN_PRIVATE, &opt);

    struct sio_http_conn *httpconn = opt.private;
    struct sio_httpprot *httpprot = httpconn->httpprot;
    struct sio_http_buffer *buf = &httpconn->buf;

    int l = buf->end - buf->last;
    l = l > len ? len : l;
    memcpy(buf->last, data, l);
    buf->last += l;

    l = buf->last - buf->pos;
    l = sio_httpprot_process(httpprot, buf->pos, l);

    if (httpconn->complete == 1) {
        buf->last = buf->pos = buf->buf; // reset
        httpconn->complete = 0;
    } else {
        buf->pos += l;
    }
    
    return 0;
}

int sio_httpmod_streamclose(sio_conn_t conn)
{
    union sio_conn_opt opt = { 0 };
    sio_conn_getopt(conn, SIO_CONN_PRIVATE, &opt);

    struct sio_http_conn *httpconn = opt.private;

    sio_httpmod_httpconn_destory(httpconn);

    return 0;
}

int sio_httpmod_destory(void)
{
    struct sio_locate *locate = sio_httpenv_get_field(locate);
    sio_locate_destory(locate);

    struct sio_locatmod *locatmod = sio_httpenv_get_field(locatmod);
    sio_locatmod_destory(locatmod);

    return 0;
}