#include "sio_doip_route.h"
#include "sio_doip_node.h"
#include "doip_hdr.h"
#include "sio_doip_err.h"
#include "sio_common.h"

int sio_doip_route_process(struct sio_doip_contbl *contbl, struct doip_route *route)
{
    unsigned short sa = ntohs(route->sa);
    if (contbl->stat == SIO_DOIP_STAT_REGISTERED_ACTIVE) {
        SIO_COND_CHECK_RETURN_VAL(contbl->sa == sa, SIO_DOIP_ERR_ROUTE_REJECT_REPEAT);
        SIO_COND_CHECK_RETURN_VAL(contbl->sa != sa, SIO_DOIP_ERR_ROUTE_REJECT_DIFFSA);
    }

    sio_doip_conn_set_state(contbl, SIO_DOIP_STAT_REGISTERED_AUTH);
    if (0) { // not auth complete
    }
    sio_doip_conn_set_state(contbl, SIO_DOIP_STAT_REGISTERED_CONFIRM);
    if (0) { // not confirm
    }

    contbl->sa = sa;
    sio_doip_conn_set_state(contbl, SIO_DOIP_STAT_REGISTERED_ACTIVE);

    return SIO_DOIP_ERR_ROUTE_OK;
}