#include "sio_stack.h"
#include <string.h>

struct sio_stack
{
    struct sio_stack_node *head;
    int count;
};

struct sio_stack *sio_stack_create()
{
    return NULL;
}

int sio_stack_push(struct sio_stack *stack, struct sio_stack_node *node)
{
    return -1;
}

int sio_stack_pop(struct sio_stack *stack)
{
    return 0;
}

struct sio_list_node *sio_stack_top(struct sio_stack *stack)
{
    return NULL;
}

int sio_stack_destory(struct sio_stack *stack)
{
    return 0;
}