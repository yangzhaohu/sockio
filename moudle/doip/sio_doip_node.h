#ifndef SIO_DOIP_NODE_H
#define SIO_DOIP_NODE_H

#define SIO_DOIP_CONN_MAX 255
#define SIO_VIN_LENGTH      17
#define SIO_ID_LENGTH       6

#define SIO_INIT_TIMEROUT_SEC         2
#define SIO_GENERAL_TIMEROUT_SEC      300

enum sio_doip_nodetype
{
    SIO_DOIP_NODETYPE_ENTITY,
    SIO_DOIP_NODETYPE_NODE
};

enum sio_doip_stat
{
    SIO_DOIP_STAT_INITIALLIZED,
    SIO_DOIP_STAT_REGISTERED_AUTH,
    SIO_DOIP_STAT_REGISTERED_CONFIRM,
    SIO_DOIP_STAT_REGISTERED_ACTIVE,
    SIO_DOIP_STAT_FINALIZE
};

struct sio_doip_contbl
{
    unsigned short sa;
    enum sio_doip_stat stat;
    struct sio_timer *inittimer;
    struct sio_timer *genetimer;
};

#define sio_doip_conn_set_state(contbl, state) contbl->stat = state

#ifdef __cplusplus
extern "C" {
#endif

int sio_doip_node_init(enum sio_doip_nodetype type);

int sio_doip_node_getcap_maxconn();
int sio_doip_node_getcap_curconn();
int sio_doip_node_getcap_process();

void sio_doip_node_setcap_curconn(int conn);

unsigned short sio_doip_node_getla();
void sio_doip_node_setla(unsigned short la);

void sio_doip_node_setgid(char id[SIO_ID_LENGTH]);
void sio_doip_node_seteid(char id[SIO_ID_LENGTH]);
void sio_doip_node_setvin(char vin[SIO_VIN_LENGTH]);

const unsigned char *sio_doip_node_getgid();
const unsigned char *sio_doip_node_geteid();
const unsigned char *sio_doip_node_getvin();

unsigned char sio_doip_node_get_sync_status();
void  sio_doip_node_set_sync_status(unsigned char status);

#ifdef __cplusplus
}
#endif

#endif