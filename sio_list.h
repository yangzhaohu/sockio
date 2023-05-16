#ifndef SIO_LIST_H_
#define SIO_LIST_H_

struct sio_list_head
{
    struct sio_list_head *next, *prev;
};

typedef struct sio_list_head sio_list_node;

static inline
void sio_list_init(struct sio_list_head *head)
{
    head->next = head->prev = head;
}

static inline
void sio_list_add(sio_list_node *node, struct sio_list_head *head)
{
    node->next = head->next;
    head->next->prev = node;

    head->next = node;
    node->prev = head;
}

static inline
void sio_list_del(sio_list_node *node)
{
    node->next->prev = node->prev;
    node->prev->next = node->next;
}

#endif