#include "sio_time.h"
#include <stdio.h>
#ifndef WIN32
#include <sys/time.h>
#include <time.h>
#else
#endif

#define SIO_TIME_FORMAT_BUFSIZE 64

#ifdef WIN32
__declspec(thread) char g_time[SIO_TIME_FORMAT_BUFSIZE] = { 0 };
#else
__thread char g_time[SIO_TIME_FORMAT_BUFSIZE] = { 0 };
#endif

#define SIO_TIME_FORMAT_BUF g_time

int sio_timezone_format(char *buf, int len)
{
    struct timeval tv;
    struct tm *t;

    gettimeofday(&tv, NULL);

    t = localtime(&tv.tv_sec);

    int l = snprintf(buf, len - 1, "%d-%02d-%02d %02d:%02d:%02d.%03ld",
        1900 + t->tm_year,
        t->tm_mon,
        t->tm_mday,
        t->tm_hour,
        t->tm_min,
        t->tm_sec,
        tv.tv_usec / 1000);

    return l;
}

const char *sio_timezone()
{
    int l = sio_timezone_format(SIO_TIME_FORMAT_BUF, SIO_TIME_FORMAT_BUFSIZE);
    SIO_TIME_FORMAT_BUF[l + 1] = 0;

    return SIO_TIME_FORMAT_BUF;
}