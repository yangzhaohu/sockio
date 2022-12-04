#include "sio_thread_pthread.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef LINUX
#include <pthread.h>
#endif
#include "sio_thread_pri.h"

#ifdef LINUX
unsigned long int sio_thread_pthread_create(struct sio_thread_pri *pri)
{
    pthread_t tid;
    if (pthread_create(&tid, NULL, pri->routine, pri->arg) != 0) {
        return -1;
    }

    return tid;
}

#endif