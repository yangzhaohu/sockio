#ifndef SIO_LOCATE_H_
#define SIO_LOCATE_H_

enum sio_locate_type
{
    SIO_LOCATE_HTTP_DIRECT,
    SIO_LOCATE_HTTP_LOCATE,
    SIO_LOCATE_HTTP_MOUDLE
};

struct sio_location
{
    int type;
    char loname[32];
    char regex[64];
    char route[256];
};

struct sio_locate;

#ifdef __cplusplus
extern "C" {
#endif

struct sio_locate *sio_locate_create();

int sio_locate_add(struct sio_locate *locate, const struct sio_location *location);

int sio_locate_remove(struct sio_locate *locate, const char *name);

int sio_locate_match(struct sio_locate *locate, const char *str, int len, struct sio_location *location);

int sio_locate_clean(struct sio_locate *locate);

int sio_locate_destory(struct sio_locate *locate);

#ifdef __cplusplus
}
#endif

#endif