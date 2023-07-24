#ifndef SIO_LOCATE_H_
#define SIO_LOCATE_H_

enum sio_http_locatype
{
    SIO_HTTP_LOCA_ROOT,
    SIO_HTTP_LOCA_INDEX,
    SIO_HTTP_LOCA_VAL,
    SIO_HTTP_LOCA_LOCA,
    SIO_HTTP_LOCA_MOD
};

struct sio_locate
{
    int type;
    char loname[32];
    char regex[64];
    char route[128];
};

#endif