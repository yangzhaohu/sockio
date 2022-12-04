#include "sio_thread_pthread.h"
#include <string.h>
#include <stdlib.h>
#ifdef WIN32
#include <process.h>
#endif
#include "sio_common.h"
#include "sio_thread_pri.h"

#ifdef WIN32

unsigned int __stdcall sio_thread_win_start_routine(void *arg)
{
    struct sio_thread_pri *pri = arg;
    pri->routine(pri->arg);
    return 0;
}

unsigned long int sio_thread_win_create(struct sio_thread_pri *pri)
{
    unsigned int tid;
    tid = _beginthreadex(NULL, 0, sio_thread_win_start_routine, pri, 0, NULL);
    SIO_COND_CHECK_RETURN_VAL(tid == 0, -1);

    return tid;
}

#endif