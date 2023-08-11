#ifndef SIO_MOD_H_
#define SIO_MOD_H_

#include "sio_submod.h"

struct sio_mod
{
    struct sio_submod submod;
    
    int (*setlocat)(const struct sio_location *locations, int size);
};

#endif