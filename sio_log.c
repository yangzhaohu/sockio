#include "sio_log.h"
#include <stdio.h>
#include <stdarg.h>
#include "sio_time.h"

static int g_level = SIO_LOG_LEVEL_INF;

#define SIO_LOGG_LEVEL g_level

void sio_logg_setlevel(int level)
{
    SIO_LOGG_LEVEL = level;
}

int sio_logg(int level, const char *format, ...)
{
    printf("[%s] ", sio_timezone());
    va_list args;
    va_start(args, format);
    int ret = vprintf(format, args);
    va_end(args);

    return ret;
}