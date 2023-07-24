#ifndef SIO_HTTPMOD_HTML_H_
#define SIO_HTTPMOD_HTML_H_

#include <stdio.h>
#include "sio_submod.h"
#include "sio_log.h"

#ifdef __cplusplus
extern "C" {
#endif

int sio_httpmod_html_response(sio_conn_t conn, char *headbuf, int size, const char *file);

#ifdef __cplusplus
}
#endif

#endif