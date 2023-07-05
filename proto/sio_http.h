#ifndef SIO_HTTP_H_
#define SIO_HTTP_H_

struct sio_http;

enum sio_http_type
{
    SIO_HTTP_REQUEST,
    SIO_HTTP_RESPONSE
};

#ifdef __cplusplus
extern "C" {
#endif

struct sio_http *sio_http_create(enum sio_http_type type);

int sio_http_append_data(struct sio_http *http, const char *data, unsigned int len);

int sio_http_destory(struct sio_http *http);

#ifdef __cplusplus
}
#endif

#endif