#include "sio_http.h"
#include <stdlib.h>
#include <string.h>
#include "sio_common.h"
#include "http_parser/http_parser.h"
#include "sio_log.h"

struct sio_http
{
    http_parser *parser;
};

static inline
int http_packet_begin(http_parser *parser)
{
    SIO_LOGE("http packet begin\n");
    return 0;
}

static inline
int http_packet_complete(http_parser *parser)
{
    SIO_LOGE("http packet complete\n");
    return 0;
}

static inline
int http_packet_url(http_parser *parser, const char *at, size_t len)
{
    SIO_LOGE("http url: %.*s\n", (int)len, at);
    return 0;
}

static inline
int http_packet_status(http_parser *parser, const char *at, size_t len)
{
    SIO_LOGE("http status: %.*s\n", (int)len, at);
    return 0;
}

static inline
int http_packet_head_field(http_parser *parser, const char *at, size_t len)
{
    SIO_LOGE("http field: %.*s\n", (int)len, at);
    return 0;
}

static inline
int http_packet_head_field_value(http_parser *parser, const char *at, size_t len)
{
    SIO_LOGE("http field_value: %.*s\n", (int)len, at);
    return 0;
}

static inline
int http_packet_header_complete(http_parser *parser)
{
    SIO_LOGE("http header complete\n");
    return 0;
}

static inline
int http_packet_chunk_header(http_parser *parser)
{
    SIO_LOGE("http chunk header\n");
    return 0;
}

static inline
int http_packet_chunk_complete(http_parser *parser)
{
    SIO_LOGE("http chunk complete\n");
    return 0;
}

static inline
int http_packet_body(http_parser *parser, const char *at, size_t len)
{
    SIO_LOGE("http body: %.*s\n", (int)len, at);
    return 0;
}

static http_parser_settings g_http_parser_cb = {
    .on_message_begin = http_packet_begin,
    .on_message_complete = http_packet_complete,
    .on_url = http_packet_url,
    .on_status = http_packet_status,
    .on_header_field = http_packet_head_field,
    .on_header_value = http_packet_head_field_value,
    .on_chunk_header = http_packet_chunk_header,
    .on_chunk_complete = http_packet_chunk_complete,
    .on_body = http_packet_body
};

struct sio_http *sio_http_create(enum sio_http_type type)
{
    struct sio_http *http = malloc(sizeof(struct sio_http));
    SIO_COND_CHECK_RETURN_VAL(!http, NULL);

    memset(http, 0, sizeof(struct sio_http));

    http_parser *parser = malloc(sizeof(http_parser));
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(!parser, NULL,
        free(http));

    enum http_parser_type hp_type = HTTP_REQUEST;
    if (type == SIO_HTTP_RESPONSE) {
        hp_type = HTTP_RESPONSE;
    }
    http_parser_init(parser, hp_type);

    http->parser = parser;

    return http;
}

int sio_http_append_data(struct sio_http *http, const char *data, unsigned int len)
{
    SIO_COND_CHECK_RETURN_VAL(!http, -1);
    http_parser_execute(http->parser, &g_http_parser_cb, data, len);

    return 0;
}

int sio_http_destory(struct sio_http *http)
{
    SIO_COND_CHECK_RETURN_VAL(!http, -1);

    free(http->parser);
    free(http);

    return 0;
}