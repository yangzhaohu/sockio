#include "sio_stack.h"
#include <stdlib.h>
#include <string.h>
#include "sio_common.h"

struct sio_stack
{
    struct sio_list_head head;
    int count;
};

struct sio_stack *sio_stack_create()
{
    struct sio_stack *stack = malloc(sizeof(struct sio_stack));
    SIO_COND_CHECK_RETURN_VAL(!stack, NULL);

    memset(stack, 0, sizeof(struct sio_stack));

    sio_list_init(&stack->head);

    return stack;
}

int sio_stack_push(struct sio_stack *stack, struct sio_list_head *entry)
{
    SIO_COND_CHECK_RETURN_VAL(!stack, -1);

    return -1;
}

int sio_stack_pop(struct sio_stack *stack)
{
    SIO_COND_CHECK_RETURN_VAL(!stack, -1);

    return 0;
}

struct sio_list_head *sio_stack_top(struct sio_stack *stack)
{
    SIO_COND_CHECK_RETURN_VAL(!stack, NULL);

    return stack->head.next;
}

int sio_stack_destory(struct sio_stack *stack)
{
    SIO_COND_CHECK_RETURN_VAL(!stack, -1);

    free(stack);

    return 0;
}