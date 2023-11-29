#include "sio_log.h"
#include <stdio.h>
#include <stdarg.h>
#include "sio_common.h"
#include "sio_global.h"
#include "sio_time.h"

static int g_level = SIO_LOG_LEVEL_INF;

#ifdef WIN32
__declspec(thread) int g_tid = -1;
#else
__thread int g_tid = -1;
#endif

#define SIO_LOGG_LEVEL g_level

#define SIO_TID (g_tid != -1 ? g_tid : sio_gettid())

void sio_logg_setlevel(int level)
{
    SIO_LOGG_LEVEL = level;
}

int sio_logg(int level, const char *format, ...)
{
    SIO_COND_CHECK_RETURN_VAL(level < SIO_LOGG_LEVEL, 0);

    printf("[%s][0x%04x] ", sio_timezone(), SIO_TID);
    va_list args;
    va_start(args, format);
    int ret = vprintf(format, args);
    va_end(args);

    return ret;
}