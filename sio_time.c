#include "sio_time.h"
#include <stdio.h>
#ifndef WIN32
#include <sys/time.h>
#include <time.h>
#else
#endif

int sio_timezone_format(char *buf, int len)
{
    struct timeval tv;
    struct tm *t;

    gettimeofday(&tv, NULL);

    t = localtime(&tv.tv_sec);

    snprintf(buf, len - 1, "%d-%02d-%02d %02d:%02d:%02d.%03ld",
        1900 + t->tm_year,
        t->tm_mon,
        t->tm_mday,
        t->tm_hour,
        t->tm_min,
        t->tm_sec,
        tv.tv_usec / 1000);

    return 0;
}