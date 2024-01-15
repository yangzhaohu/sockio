#include "sio_log.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#ifdef WIN32
#include <malloc.h>
#endif
#include "sio_common.h"
#include "sio_def.h"
#include "sio_global.h"
#include "sio_time.h"

const char *g_logg_level_str[] = {
    [SIO_LOG_LEVEL_TRC] = "TRC",
    [SIO_LOG_LEVEL_DBG] = "DBG",
    [SIO_LOG_LEVEL_INF] = "INF",
    [SIO_LOG_LEVEL_WAR] = "WAR",
    [SIO_LOG_LEVEL_ERR] = "ERR"
};

static int g_level = SIO_LOG_LEVEL_INF;
static int g_prefix_enable = 1;

#define SIO_LOGG_LEVEL g_level
#define SIO_LOGG_PREFIX_ENABLE g_prefix_enable

static sio_tls_t char g_prefix_buf[256] = { 0 };

static inline
const char *sio_logg_prefix(int level, int *len)
{
#define SIO_LOGG_LEVEL_STR(level) g_logg_level_str[level]
    const char *lstr = "UKN";
    if (SIO_LOG_LEVEL_TRC <= level && level <= SIO_LOG_LEVEL_ERR) {
        lstr = SIO_LOGG_LEVEL_STR(level);
    }
    *len = snprintf(g_prefix_buf, 255, "[%s][0x%x][%s]",
        sio_timezone(), sio_gettid(), lstr);

    return g_prefix_buf;
}

void sio_logg_setlevel(int level)
{
    SIO_LOGG_LEVEL = level;
}

void sio_logg_enable_prefix(int enable)
{
    g_prefix_enable = enable != 0 ? 1 : 0;
}

int sio_logg(int level, const char *format, ...)
{
    SIO_COND_CHECK_RETURN_VAL(level < SIO_LOGG_LEVEL || format == NULL, 0);

    int ret = 0;
    va_list args;
    va_start(args, format);

    if (SIO_LOGG_PREFIX_ENABLE) {
        int l = strlen(format);
        int prelen = 0;
        const char *prefix = sio_logg_prefix(level, &prelen);

#ifdef WIN32
        char *fmt = _alloca(l + prelen + 1);
#else
        char fmt[l + prelen + 1];
#endif
        fmt[l + prelen] = 0;

        memcpy(fmt, prefix, prelen);
        memcpy(fmt + prelen, format, l);

        ret = vprintf(fmt, args);
    } else {
        ret = vprintf(format, args);
    }

    va_end(args);

    return ret;
}