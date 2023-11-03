#include "sio_doip_node.h"
#include <string.h>
#include "sio_common.h"

struct sio_doip_conf
{
    unsigned char vin[SIO_VIN_LENGTH + 1];
    unsigned char gid[SIO_ID_LENGTH + 1];
    unsigned char eid[SIO_ID_LENGTH + 1];
};

struct sio_doip_capital
{
    unsigned char maxconn; // max connection
    unsigned char curconn; // cur connection
    unsigned int  process;
};

struct sio_doip_statistics
{
    enum sio_doip_nodetype type;
    unsigned short la;

    struct sio_doip_conf    conf;
    struct sio_doip_capital cap;

    unsigned char sync;
};

static struct sio_doip_statistics g_statis = {
    .type = -1,
    .la = 0,
    .conf = {
        .vin = "",
        .gid = "",
        .eid = ""
    },
    .cap = {
        .maxconn = 8,
        .curconn = 0,
        .process = 8
    },
    .sync = 1,
};

#define SIO_DOIP_STATIS                g_statis
#define SIO_DOIP_STATIS_CONF           SIO_DOIP_STATIS.conf
#define SIO_DOIP_STATIS_CAP            SIO_DOIP_STATIS.cap
#define SIO_DOIP_STATIS_SYNC_STATUS    SIO_DOIP_STATIS.sync

int sio_doip_node_init(enum sio_doip_nodetype type)
{
    SIO_COND_CHECK_RETURN_VAL(type < SIO_DOIP_NODETYPE_ENTITY || type > SIO_DOIP_NODETYPE_NODE, -1);

    SIO_DOIP_STATIS.type = type;

    return 0;
}

int sio_doip_node_getcap_maxconn()
{
    return SIO_DOIP_STATIS_CAP.maxconn;
}

int sio_doip_node_getcap_curconn()
{
    return SIO_DOIP_STATIS_CAP.curconn;
}

int sio_doip_node_getcap_process()
{
    return SIO_DOIP_STATIS_CAP.process;
}

void sio_doip_node_setcap_curconn(int conn)
{
    SIO_DOIP_STATIS_CAP.curconn = conn;
}

unsigned short sio_doip_node_getla()
{
    return SIO_DOIP_STATIS.la;
}

void sio_doip_node_setla(unsigned short la)
{
    SIO_DOIP_STATIS.la = la;
}

void sio_doip_node_setgid(char id[SIO_ID_LENGTH])
{
    memcpy(SIO_DOIP_STATIS_CONF.gid, id, SIO_ID_LENGTH);
}

void sio_doip_node_seteid(char id[SIO_ID_LENGTH])
{
    memcpy(SIO_DOIP_STATIS_CONF.eid, id, SIO_ID_LENGTH);
}

void sio_doip_node_setvin(char vin[SIO_VIN_LENGTH])
{
    memcpy(SIO_DOIP_STATIS_CONF.vin, vin, SIO_VIN_LENGTH);
}

const unsigned char *sio_doip_node_getgid()
{
    return SIO_DOIP_STATIS_CONF.gid;
}

const unsigned char *sio_doip_node_geteid()
{
    return SIO_DOIP_STATIS_CONF.eid;
}

const unsigned char *sio_doip_node_getvin()
{
    return SIO_DOIP_STATIS_CONF.vin;
}

unsigned char sio_doip_node_get_sync_status()
{
    return SIO_DOIP_STATIS_SYNC_STATUS;
}

void sio_doip_node_set_sync_status(unsigned char status)
{
    SIO_DOIP_STATIS_SYNC_STATUS = status;
}