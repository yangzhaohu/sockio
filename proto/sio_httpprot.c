#include "sio_httpprot.h"
#include <stdlib.h>
#include <string.h>
#include "sio_common.h"
#include "http_parser/http_parser.h"
#include "sio_log.h"

struct sio_httpprot_owner
{
    void *private;
    struct sio_httpprot_ops ops;
};

struct sio_httpprot
{
    http_parser parser;
    struct sio_httpprot_owner owner;
};

#define sio_httpprot_callcb(type, at, len) \
    do { \
        struct sio_httpprot *httpprot = (struct sio_httpprot *)parser; \
        struct sio_httpprot_owner *owner = &httpprot->owner; \
        if (owner->ops.prot_data != NULL) { \
            int ret = owner->ops.prot_data(owner->private, type, at, len); \
            if (ret == -1) { \
                http_parser_pause(parser, 1); \
            } \
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

    int ret = http_parser_execute(&httpprot->parser, &g_http_parser_cb, data, len);
    if (ret == 0 || httpprot->parser.http_errno != 0) {
        printf("ret: %d, err: %d\n", ret, httpprot->parser.http_errno);
    }
    return ret;
}

int sio_httpprot_destory(struct sio_httpprot *httpprot)
{
    SIO_COND_CHECK_RETURN_VAL(!httpprot, -1);

    free(httpprot);

    return 0;
}