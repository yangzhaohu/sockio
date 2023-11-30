#include "sio_log.h"
#include <stdio.h>
#include <stdarg.h>
#include "sio_common.h"

static int g_level = SIO_LOG_LEVEL_INF;

#define SIO_LOGG_LEVEL g_level

void sio_logg_setlevel(int level)
{
    SIO_LOGG_LEVEL = level;
}

int sio_logg(int level, const char *format, ...)
{
    SIO_COND_CHECK_RETURN_VAL(level < SIO_LOGG_LEVEL, 0);

    va_list args;
    va_start(args, format);
    int ret = vprintf(format, args);
    va_end(args);

    return ret;
}