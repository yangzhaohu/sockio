#include "sio_thread_pthread.h"
#include <string.h>
#include <stdlib.h>
#ifdef WIN32
#include <windows.h>
#include <process.h>
#endif
#include "sio_common.h"
#include "sio_thread_pri.h"
#include "sio_log.h"

#ifdef WIN32

unsigned int __stdcall sio_thread_win_start_routine(void *arg)
{
    struct sio_thread_pri *pri = arg;
    pri->routine(pri->arg);
    return 0;
}

sio_uptr_t sio_thread_win_create(struct sio_thread_pri *pri)
{
    sio_uptr_t tid;
    tid = _beginthreadex(NULL, 0, sio_thread_win_start_routine, pri, 0, NULL);
    SIO_COND_CHECK_RETURN_VAL(tid == 0, -1);

    return tid;
}

int sio_thread_win_join(sio_uptr_t tid)
{
    return WaitForSingleObject((HANDLE)tid, INFINITE);
}

int sio_thread_win_destory(sio_uptr_t tid)
{
    int ret = sio_thread_win_join(tid);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret != WAIT_OBJECT_0, -1,
        SIO_LOGE("_beginthreadex  WaitForSingleObject err: %d", ret));

    return 0;
}

#endif