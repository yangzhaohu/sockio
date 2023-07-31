#ifndef SIO_LOCATMOD_H_
#define SIO_LOCATMOD_H_

struct sio_locatmod;

#ifdef __cplusplus
extern "C" {
#endif

struct sio_locatmod *sio_locatmod_create();

int sio_locatmod_add(struct sio_locatmod *locatmod, const char *name, void *data);

int sio_locatmod_remove(struct sio_locatmod *locatmod, const char *name);

void *sio_locatmod_cat(struct sio_locatmod *locatmod, const char *name);

int sio_locatmod_clean(struct sio_locatmod *locatmod);

int sio_locatmod_destory(struct sio_locatmod *locatmod);

#ifdef __cplusplus
}
#endif

#endif