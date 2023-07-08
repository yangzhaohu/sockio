#ifndef SIO_HTTPMOD_H_
#define SIO_HTTPMOD_H_

#ifdef __cplusplus
extern "C" {
#endif

int sio_httpmod_type();
int sio_httpmod_version(const char **version);

int sio_httpmod_create(void **mod);

int sio_httpmod_streamin(void *mod, const char *data, int len);

int sio_httpmod_destory(void *mod);

#ifdef __cplusplus
}
#endif

#endif