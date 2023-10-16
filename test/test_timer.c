#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "sio_global.h"
#include "sio_timer.h"

static inline
void *timer_routine(void *arg)
{
    printf("timer trigger\n");
    return NULL;
}

int main()
{
    struct sio_timer *timer = sio_timer_create(timer_routine, NULL);
    sio_timer_start(timer, 1000 * 1000 * 4, 1000 * 1000 * 1);

    sio_global_sigaction(SIGUSR1);

    while (1) {
        char c = getc(stdin);
        if (c == 'q') {
            break;
        } else if (c == 's') {
            kill(getpid(), SIGUSR1);
        }
    }

    return 0;
}