#ifndef SIO_LOG_H_
#define SIO_LOG_H_

#define SIO_LOG_LEVEL_TRC     1    // trace
#define SIO_LOG_LEVEL_DBG     2    // debug
#define SIO_LOG_LEVEL_INF     3    // info
#define SIO_LOG_LEVEL_WAR     4    // warn
#define SIO_LOG_LEVEL_ERR     5    // error


#define SIO_LOGT(format, ...) sio_logg(SIO_LOG_LEVEL_TRC, format, ##__VA_ARGS__)
#define SIO_LOGD(format, ...) sio_logg(SIO_LOG_LEVEL_DBG, format, ##__VA_ARGS__)
#define SIO_LOGI(format, ...) sio_logg(SIO_LOG_LEVEL_ERR, format, ##__VA_ARGS__)
#define SIO_LOGW(format, ...) sio_logg(SIO_LOG_LEVEL_WAR, format, ##__VA_ARGS__)
#define SIO_LOGE(format, ...) sio_logg(SIO_LOG_LEVEL_ERR, format, ##__VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

void sio_logg_setlevel(int level);

void sio_logg_enable_prefix(int enable);

int sio_logg(int level, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif