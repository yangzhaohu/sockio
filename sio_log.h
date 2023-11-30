#ifndef SIO_LOG_H_
#define SIO_LOG_H_

#include "sio_common.h"
#include "sio_global.h"
#include "sio_time.h"

#define SIO_LOG_LEVEL_TRC     1    // trace
#define SIO_LOG_LEVEL_DBG     2    // debug
#define SIO_LOG_LEVEL_INF     3    // info
#define SIO_LOG_LEVEL_WAR     4    // warn
#define SIO_LOG_LEVEL_ERR     5    // error

#define SIO_LOG_LEVEL_DES1  "TRC"
#define SIO_LOG_LEVEL_DES2  "DBG"
#define SIO_LOG_LEVEL_DES3  "INF"
#define SIO_LOG_LEVEL_DES4  "WAR"
#define SIO_LOG_LEVEL_DES5  "ERR"

#define SIO_LOG_LEVEL_STRING(level)  SIO_CAT_STR(SIO_LOG_LEVEL_DES, level)

#define SIO_LOG_IMP(level, format, ...) sio_logg(level,                 \
    "[%s][0x%x][%s]" format,                                            \
    sio_timezone(), sio_gettid(), SIO_LOG_LEVEL_STRING(level),          \
    ##__VA_ARGS__)

#define SIO_LOGT(format, ...) SIO_LOG_IMP(SIO_LOG_LEVEL_TRC, format, ##__VA_ARGS__)
#define SIO_LOGD(format, ...) SIO_LOG_IMP(SIO_LOG_LEVEL_DBG, format, ##__VA_ARGS__)
#define SIO_LOGI(format, ...) SIO_LOG_IMP(SIO_LOG_LEVEL_ERR, format, ##__VA_ARGS__)
#define SIO_LOGW(format, ...) SIO_LOG_IMP(SIO_LOG_LEVEL_WAR, format, ##__VA_ARGS__)
#define SIO_LOGE(format, ...) SIO_LOG_IMP(SIO_LOG_LEVEL_ERR, format, ##__VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

void sio_logg_setlevel(int level);

int sio_logg(int level, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif