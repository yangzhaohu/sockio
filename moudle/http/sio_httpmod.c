#include "sio_httpmod.h"
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include "sio_common.h"
#include "sio_def.h"
#include "sio_list.h"
#include "proto/sio_httpprot.h"
#include "sio_httpmod_html.h"
#include "sio_log.h"

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
    struct sio_http_buffer buf;
    struct sio_httpprot *httpprot;
    struct sio_httpds ds[SIO_HTTPDS_BUTT];

    unsigned int complete:1; // packet complete
};

struct sio_http_realloca
{
    struct sio_list_head entry;
    struct sio_locate loca;
    union {
        struct sio_submod *submod;
        struct sio_locate *loca;
    } route;
};

struct sio_http_submod
{
    struct sio_list_head entry;
    struct sio_submod *submod;
};

struct sio_http_env
{
    char root[256];
    char index[256];

    struct sio_list_head locahead;
    struct sio_list_head submodhead;
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
    struct sio_http_conn *httpconn = handler;
    struct sio_httpds *ds = httpconn->ds;

    if (type == SIO_HTTPPROTO_STAT_COMPLETE) {
        httpconn->complete = 1;
        const char *url = ds[SIO_HTTPDS_URL].dsstr.data;
        if (strncmp(url, "/", strlen("/")) == 0) {
            char name[512] = { 0 };
            sprintf(name, "%sindex.html", sio_httpenv_get_field(root));
            char head[4096] = { 0 };
            sio_httpmod_html_response(httpconn->conn, head, 4096, name);
            if (sio_httpmod_conn_keep(httpconn) != 0) {
                sio_conn_close(httpconn->conn);
            }
        }
    }

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
    }

    return len;
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

static inline
struct sio_submod *sio_httpmod_find_submod(const char *name)
{
    struct sio_http_submod *submod = NULL;
    struct sio_list_head *pos;
    sio_list_foreach(pos, &sio_httpenv_get_field(submodhead)) {
        submod = (struct sio_http_submod *)pos;
        if (submod->submod->mod_name) {
            const char *modname = NULL;
            submod->submod->mod_name(&name);
            int ret = strcmp(modname, name);
            if (ret == 0) {
                break;
            }
        }
    }

    if (submod) {
        return submod->submod;
    }

    return NULL;
}

int sio_httpmod_create(void)
{
    sio_list_init(&sio_httpenv_get_field(locahead));
    sio_list_init(&sio_httpenv_get_field(submodhead));

    return 0;
}

int sio_httpmod_setlocat(const struct sio_locate *locations, int size)
{
    for (int i = 0; i < size; i++) {
        const struct sio_locate *loca = &locations[i];
        struct sio_submod *submod = NULL;
        int len = strlen(loca->route);
        len = len >= 256 ? 255 : len;
        if (loca->type == SIO_HTTP_LOCA_ROOT) {
            memcpy(sio_httpenv_get_field(root), loca->route, len);
        } else if (loca->type == SIO_HTTP_LOCA_INDEX) {
            memcpy(sio_httpenv_get_field(index), loca->route, len);
        } else if (loca->type == SIO_HTTP_LOCA_MOD) {
            submod = sio_httpmod_find_submod(loca->route);
            if (submod == NULL) {
                SIO_LOGE("not found moudle: %s\n", loca->route);
                continue;
            }
        }

        struct sio_http_realloca *realloca = malloc(sizeof(struct sio_http_realloca));
        memset(realloca, 0, sizeof(struct sio_http_realloca));
        memcpy(&realloca->loca, loca, sizeof(struct sio_locate));
        if (loca->type == SIO_HTTP_LOCA_MOD) {
            realloca->route.submod = submod;
        }
        sio_list_add(&realloca->entry, &sio_httpenv_get_field(locahead));
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
    return 0;
}