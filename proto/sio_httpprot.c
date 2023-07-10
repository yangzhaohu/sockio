#include "sio_httpprot.h"
#include <stdlib.h>
#include <string.h>
#include "sio_common.h"
#include "http_parser/http_parser.h"
#include "sio_log.h"

typedef struct
{
    int len;
    const char *data;
}sio_str_t;

struct sio_header_buffer
{
    char *buf;
    char *cur;
    unsigned int length;
};

struct sio_body_buffer
{
    char *buf;
    char *cur;
    unsigned int length;
};

struct sio_http_headers
{
    sio_str_t status;
    sio_str_t uri;
};

struct sio_http_body
{
    sio_str_t data;
};

struct sio_http_stat
{
    unsigned int methond;
    unsigned int keepalive;
};

struct sio_http_conn
{
    struct sio_http_headers header;
    struct sio_http_body body;

    struct sio_http_stat stat;
};

struct sio_httpprot_owner
{
    void *private;
    struct sio_httpprot_ops ops;
};

struct sio_httpprot
{
    http_parser parser;
    struct sio_httpprot_owner owner;
    struct sio_http_conn conn;
    struct sio_header_buffer headbuf;
    struct sio_body_buffer bodybuf;
};

#define sio_httpprot_callcb(type, at, len) \
    do { \
        struct sio_httpprot *httpprot = (struct sio_httpprot *)parser; \
        struct sio_httpprot_owner *owner = &httpprot->owner; \
        if (owner->ops.prot_data != NULL) { \
            owner->ops.prot_data(owner->private, type, at, len); \
        } \
    } while (0);
    

static inline
int http_packet_begin(http_parser *parser)
{
    static char data[1] = { 0 };
    sio_httpprot_callcb(SIO_HTTPPROTO_VALTYPE_BEGIN, data, 0);
    return 0;
}

static inline
int http_packet_complete(http_parser *parser)
{
    static char data[1] = { 0 };
    sio_httpprot_callcb(SIO_HTTPPROTO_VALTYPE_COMPLETE, data, 0);
    return 0;
}

static inline
int http_packet_url(http_parser *parser, const char *at, size_t len)
{
    sio_httpprot_callcb(SIO_HTTPPROTO_VALTYPE_URL, at, len);
    return 0;
}

static inline
int http_packet_status(http_parser *parser, const char *at, size_t len)
{
    sio_httpprot_callcb(SIO_HTTPPROTO_VALTYPE_STATUS, at, len);
    return 0;
}

static inline
int http_packet_head_field(http_parser *parser, const char *at, size_t len)
{
    sio_httpprot_callcb(SIO_HTTPPROTO_VALTYPE_HEADFIELD, at, len);
    return 0;
}

static inline
int http_packet_head_field_value(http_parser *parser, const char *at, size_t len)
{
    sio_httpprot_callcb(SIO_HTTPPROTO_VALTYPE_HEADVALUE, at, len);
    return 0;
}

static inline
int http_packet_header_complete(http_parser *parser)
{
    static char data[1] = { 0 };
    sio_httpprot_callcb(SIO_HTTPPROTO_VALTYPE_HEADCOMPLETE, data, 0);
    return 0;
}

static inline
int http_packet_chunk_header(http_parser *parser)
{
    static char data[1] = { 0 };
    sio_httpprot_callcb(SIO_HTTPPROTO_VALTYPE_CHUNKHEAD, data, 0);
    return 0;
}

static inline
int http_packet_chunk_complete(http_parser *parser)
{
    static char data[1] = { 0 };
    sio_httpprot_callcb(SIO_HTTPPROTO_VALTYPE_CHUNKCOMPLETE, data, 0);
    return 0;
}

static inline
int http_packet_body(http_parser *parser, const char *at, size_t len)
{
    sio_httpprot_callcb(SIO_HTTPPROTO_VALTYPE_BODY, at, len);
    return 0;
}

static http_parser_settings g_http_parser_cb = {
    .on_message_begin = http_packet_begin,
    .on_message_complete = http_packet_complete,
    .on_url = http_packet_url,
    .on_status = http_packet_status,
    .on_header_field = http_packet_head_field,
    .on_header_value = http_packet_head_field_value,
    .on_headers_complete = http_packet_header_complete,
    .on_chunk_header = http_packet_chunk_header,
    .on_chunk_complete = http_packet_chunk_complete,
    .on_body = http_packet_body
};

struct sio_httpprot *sio_httpprot_create(enum sio_httpprot_type type)
{
    struct sio_httpprot *httpprot = malloc(sizeof(struct sio_httpprot));
    SIO_COND_CHECK_RETURN_VAL(!httpprot, NULL);

    memset(httpprot, 0, sizeof(struct sio_httpprot));

    enum http_parser_type hp_type = HTTP_REQUEST;
    if (type == SIO_HTTP_RESPONSE) {
        hp_type = HTTP_RESPONSE;
    }
    http_parser_init(&httpprot->parser, hp_type);

#define SIO_HTTP_HEAD_BUFFER_SIZE 4096 * 4
    char *head_buf = malloc(SIO_HTTP_HEAD_BUFFER_SIZE);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!head_buf, NULL,
        free(httpprot));

    memset(head_buf, 0, SIO_HTTP_HEAD_BUFFER_SIZE);

    httpprot->headbuf.buf = head_buf;
    httpprot->headbuf.cur = head_buf;
    httpprot->headbuf.length = SIO_HTTP_HEAD_BUFFER_SIZE;

#define SIO_HTTP_BODY_BUFFER_SIZE 4096 * 4
    char *body_buf = malloc(SIO_HTTP_BODY_BUFFER_SIZE);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!body_buf, NULL,
        free(head_buf),
        free(httpprot));

    memset(body_buf, 0, SIO_HTTP_BODY_BUFFER_SIZE);

    httpprot->bodybuf.buf = body_buf;
    httpprot->bodybuf.cur = body_buf;
    httpprot->bodybuf.length = SIO_HTTP_BODY_BUFFER_SIZE;

    return httpprot;
}

int sio_httpprot_setopt(struct sio_httpprot *httpprot, enum sio_httpprot_optcmd cmd, union sio_httpprot_opt *opt)
{
    SIO_COND_CHECK_RETURN_VAL(!httpprot, -1);
    struct sio_httpprot_owner *owner = &httpprot->owner;

    int ret = 0;
    switch (cmd) {
    case SIO_HTTPPROT_PRIVATE:
        owner->private = opt->private;
        break;

    case SIO_HTTPPROT_OPS:
        owner->ops = opt->ops;
        break;
    
    default:
        ret = -1;
        break;
    }

    return ret;
}

int sio_httpprot_process(struct sio_httpprot *httpprot, const char *data, unsigned int len)
{
    SIO_COND_CHECK_RETURN_VAL(!httpprot, -1);
    http_parser_execute(&httpprot->parser, &g_http_parser_cb, data, len);

    return 0;
}

int sio_httpprot_destory(struct sio_httpprot *httpprot)
{
    SIO_COND_CHECK_RETURN_VAL(!httpprot, -1);

    free(httpprot);

    return 0;
}