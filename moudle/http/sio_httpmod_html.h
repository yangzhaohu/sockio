#ifndef SIO_HTTPMOD_HTML_H_
#define SIO_HTTPMOD_HTML_H_

struct sio_socket;

#ifdef __cplusplus
extern "C" {
#endif

int sio_httpmod_html_response(struct sio_socket *sock, char *headbuf, int size, const char *file);

#ifdef __cplusplus
}
#endif

#endif