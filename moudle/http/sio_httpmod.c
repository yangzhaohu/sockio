#include "sio_httpmod.h"
#include <string.h>
#include <stdlib.h>
#include "sio_common.h"
#include "sio_server.h"
#include "sio_list.h"
#include "moudle/sio_locate.h"
#include "moudle/sio_locatmod.h"
#include "proto/sio_httpprot.h"
#include "sio_httpmod_html.h"
#include "sio_log.h"

struct sio_httpmod
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
    struct sio_httpmod *mod;
    struct sio_socket *sock;
    int matchpos;
    struct sio_location location;
    struct sio_http_buffer buf;
    struct sio_httpprot *httpprot;
    struct sio_httpds ds[SIO_HTTPDS_BUTT];

    unsigned int complete:1; // packet complete
};

int sio_httpmod_name(const char **name)
{
    *name = "sio_http_core_module";
    return 0;
}

int sio_httpmod_type(void)
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
    // SIO_LOGE("type: %d, %.*s\n", type, (int)data->length, data->data);

    int len = data->length;
    struct sio_http_conn *httpconn = handler;
    struct sio_httpds *ds = httpconn->ds;

    if (type == SIO_HTTPPROTO_DATA_URL) {
        if (len > 4096) {
            sio_socket_write(httpconn->sock, g_resp, strlen(g_resp));
            sio_socket_close(httpconn->sock);
            SIO_LOGI("url too large\n");
            return -1;
        }
        ds[SIO_HTTPDS_URL].dsstr.data = data->data;
        ds[SIO_HTTPDS_URL].dsstr.length = len;

        struct sio_locate *locate = httpconn->mod->locate;
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
        char head[4096] = { 0 };
        sio_httpmod_html_response(httpconn->sock, head, 4096, name);
        if (sio_httpmod_conn_keep(httpconn) != 0) {
            sio_socket_close(httpconn->sock);
        }
    }

    return 0;
}

static inline
struct sio_http_conn *sio_httpmod_httpconn_create(struct sio_httpmod *mod)
{
    struct sio_http_conn *httpconn = malloc(sizeof(struct sio_http_conn));
    SIO_COND_CHECK_RETURN_VAL(!httpconn, NULL);

    memset(httpconn, 0, sizeof(struct sio_http_conn));

    httpconn->mod = mod;

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

    return httpconn;
}

static inline
void sio_httpmod_httpconn_destory(struct sio_http_conn *httpconn)
{
    sio_httpprot_destory(httpconn->httpprot);
    
    free(httpconn->buf.buf);
    free(httpconn);
}

static inline
int sio_httpmod_socket_readable(struct sio_socket *sock)
{
    union sio_sockopt opt = { 0 };
    sio_socket_getopt(sock, SIO_SOCK_PRIVATE, &opt);

    struct sio_http_conn *hconn = opt.private;
    struct sio_httpprot *httpprot = hconn->httpprot;
    struct sio_http_buffer *buf = &hconn->buf;

    int l = buf->end - buf->last;
    int len = sio_socket_read(sock, buf->last, l);
    SIO_COND_CHECK_RETURN_VAL(len <= 0, 0);
    buf->last += len;

    l = buf->last - buf->pos;
    l = sio_httpprot_process(httpprot, buf->pos, l);

    if (hconn->complete == 1) {
        buf->last = buf->pos = buf->buf; // reset
        hconn->complete = 0;
    } else {
        buf->pos += l;
    }

    return 0;
}

static inline
int sio_httpmod_socket_writeable(struct sio_socket *sock)
{
    return 0;
}

static inline
int sio_httpmod_socket_closeable(struct sio_socket *sock)
{
    union sio_sockopt opt = { 0 };
    sio_socket_getopt(sock, SIO_SOCK_PRIVATE, &opt);

    struct sio_http_conn *hconn = opt.private;
    sio_httpmod_httpconn_destory(hconn);

    sio_socket_destory(sock);

    return 0;
}

sio_submod_t sio_httpmod_create(void)
{
    struct sio_httpmod *mod = malloc(sizeof(struct sio_httpmod));
    SIO_COND_CHECK_RETURN_VAL(!mod, NULL);

    memset(mod, 0, sizeof(struct sio_httpmod));

    struct sio_locate *locate = sio_locate_create();
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!locate, NULL,
        free(mod));

    struct sio_locatmod *locatmod = sio_locatmod_create();
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!locate, NULL,
        sio_locate_destory(locate),
        free(mod));

    mod->locate = locate;
    mod->locatmod = locatmod;

    return mod;
}

int sio_httpmod_setlocate(sio_submod_t mod, const struct sio_location *locations, int size)
{
    struct sio_locate *locate = ((struct sio_httpmod *)mod)->locate;
    for (int i = 0; i < size; i++) {
        sio_locate_add(locate, &locations[i]);
    }

    return 0;
}

int sio_httpmod_modhook(sio_submod_t mod, struct sio_submod *submod)
{
    return 0;
}

int sio_httpmod_newconn(sio_submod_t mod, struct sio_server *server)
{
    struct sio_socket *sock = sio_socket_create(SIO_SOCK_TCP, NULL);
    union sio_sockopt opt = {
        .ops.readable = sio_httpmod_socket_readable,
        .ops.writeable = sio_httpmod_socket_writeable,
        .ops.closeable = sio_httpmod_socket_closeable
    };
    sio_socket_setopt(sock, SIO_SOCK_OPS, &opt);

    opt.nonblock = 1;
    sio_socket_setopt(sock, SIO_SOCK_NONBLOCK, &opt);

    int ret = sio_server_accept(server, sock);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        sio_socket_destory(sock));

    struct sio_http_conn *hconn = sio_httpmod_httpconn_create(mod);
    hconn->sock = sock;

    opt.private = hconn;
    sio_socket_setopt(sock, SIO_SOCK_PRIVATE, &opt);

    ret = sio_server_socket_mplex(server, sock);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        sio_socket_destory(sock));

    return 0;
}

int sio_httpmod_destory(sio_submod_t mod)
{
    struct sio_locate *locate = ((struct sio_httpmod *)mod)->locate;
    sio_locate_destory(locate);

    struct sio_locatmod *locatmod = ((struct sio_httpmod *)mod)->locatmod;
    sio_locatmod_destory(locatmod);

    free(mod);

    return 0;
}