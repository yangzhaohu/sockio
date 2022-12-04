#ifndef SIO_LOG_H_
#define SIO_LOG_H_

#include <stdio.h>

#define SIO_LOGE(format, ...) printf(format, ##__VA_ARGS__)

#endif