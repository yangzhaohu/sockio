#include "sio_global.h"
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include <windows.h>
#include <winsock2.h>
#else
#include <signal.h>
#include <unistd.h>
#include <sys/syscall.h>
#endif
#include "sio_common.h"
#include "sio_log.h"

#ifdef WIN32

static inline
void sio_winsock_init(void)
{
    WORD version;
    WSADATA data;

    version = MAKEWORD(2, 2);
    int err = WSAStartup(version, &data);
    SIO_COND_CHECK_CALLOPS(err != 0,
        SIO_LOGE("WSAStartup failed with error: %d\n", err));
}

int sio_global_sigaction(int sig)
{
    return -1;
}

int sio_global_sigmask(int sig, int how)
{
    return -1;
}

int sio_global_sigwait(int sig)
{
    return -1;
}

int sio_global_sigwaitinfo(int sig, void *sigptr)
{
    return -1;
}

#else
static inline
void sio_signal_proc(int sig)
{
    SIO_LOGI("sio_signal_proc sig: %d\n", sig);
}

int sio_global_sigaction(int sig)
{
    struct sigaction sa = { 0 };
    sa.sa_handler = sio_signal_proc;
    int ret = sigaction(sig, &sa, NULL);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        SIO_LOGE("sigaction failed\n"));

    return 0;
}

int sio_global_sigmask(int sig, int how)
{
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, sig);
    int ret = sigprocmask(how, &sigset, NULL);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        SIO_LOGE("sigprocmask failed\n"));

    return 0;
}

int sio_global_sigwait(int sig)
{
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, sig);
    int ret = sigwait(&sigset, &sig);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret != 0, -1,
        SIO_LOGE("sigwait error\n"));

    return sig;
}

int sio_global_sigwaitinfo(int sig, void *sigptr)
{
    sigset_t sigset;
    siginfo_t info = { 0 };
    sigemptyset(&sigset);
    sigaddset(&sigset, sig);
    int ret = sigwaitinfo(&sigset, &info);
    SIO_COND_CHECK_CALLOPS_RETURN_VAL(ret == -1, -1,
        SIO_LOGE("sigwaitinfo error\n"));
    
    (*(void **)sigptr) = info.si_value.sival_ptr;

    return ret;
}

#endif

class sio_global_init
{
public:
    sio_global_init(/* args */);
    ~sio_global_init();
};

sio_global_init::sio_global_init(/* args */)
{
#ifdef WIN32
    sio_winsock_init();
#endif

#ifdef WIN32
#else
    sio_global_sigmask(SIGALRM, SIG_BLOCK);
#endif
}

sio_global_init::~sio_global_init()
{
}

static sio_global_init g_global_init;


int sio_gettid()
{
#ifdef WIN32
    int tid = GetCurrentThreadId();
#else
    pid_t tid = syscall(SYS_gettid);
#endif
    return tid;
}