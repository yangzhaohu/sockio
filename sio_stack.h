#ifndef SIO_STACK_H_
#define SIO_STACK_H_

struct sio_stack;

struct sio_stack_node
{
    void *data;
    struct sio_stack_node *next;
};

#ifdef __cplusplus
extern "C" {
#endif

struct sio_stack *sio_stack_create();

int sio_stack_push(struct sio_stack *stack, struct sio_stack_node *node);

int sio_stack_pop(struct sio_stack *stack);

struct sio_list_node *sio_stack_top(struct sio_stack *stack);

int sio_stack_destory(struct sio_stack *stack);

#ifdef __cplusplus
}
#endif

#endif