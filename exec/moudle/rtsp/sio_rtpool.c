#include "sio_rtpool.h"
#include <stdlib.h>
#include <string.h>
#include "sio_common.h"
#include "sio_list.h"
#include "sio_log.h"

struct sio_rtpool_node
{
    struct sio_list_head entry;
    char name[256];
    void *stream;
};

typedef struct sio_list_head sio_rtpool_head;

struct sio_rtpool
{
    sio_rtpool_head head;
};

struct sio_rtpool *sio_rtpool_create()
{
    struct sio_rtpool *rtpool = malloc(sizeof(struct sio_rtpool));
    SIO_COND_CHECK_RETURN_VAL(!rtpool, NULL);

    memset(rtpool, 0, sizeof(struct sio_rtpool));

    sio_list_init(&rtpool->head);

    return rtpool;
}

static inline
struct sio_rtpool_node *sio_rtpool_find_imp(struct sio_list_head *head, const char *name)
{
    struct sio_rtpool_node *node = NULL;
    struct sio_list_head *pos;
    sio_list_foreach(pos, head) {
        node = (struct sio_rtpool_node *)pos;
        if (strcmp(node->name, name) == 0) {
            return node;
        }
    }

    return NULL;
}

int sio_rtpool_add(struct sio_rtpool *rtpool, const char *name, void *stream)
{
    SIO_COND_CHECK_RETURN_VAL(!rtpool, -1);

    struct sio_rtpool_node *node = NULL;
    node = sio_rtpool_find_imp(&rtpool->head, name);
    SIO_COND_CHECK_RETURN_VAL(node, -1);

    node = malloc(sizeof(struct sio_rtpool_node));
    SIO_COND_CHECK_RETURN_VAL(!node, -1);
    memset(node, 0, sizeof(struct sio_rtpool_node));

    int l = strlen(name);
    SIO_COND_CHECK_RETURN_VAL(l > 255, -1);

    memcpy(node->name, name, l);
    node->stream = stream;

    sio_list_add(&node->entry, &rtpool->head);

    return 0;
}

int sio_rtpool_find(struct sio_rtpool *rtpool, const char *name, void **stream)
{
    SIO_COND_CHECK_RETURN_VAL(!rtpool,-1);

    struct sio_rtpool_node *node = NULL;
    node = sio_rtpool_find_imp(&rtpool->head, name);
    SIO_COND_CHECK_RETURN_VAL(!node, -1);

    *stream = node->stream;

    return 0;
}

int sio_rtpool_del(struct sio_rtpool *rtpool, const char *name)
{
    SIO_COND_CHECK_RETURN_VAL(!rtpool,-1);

    struct sio_rtpool_node *node = NULL;
    node = sio_rtpool_find_imp(&rtpool->head, name);
    SIO_COND_CHECK_RETURN_VAL(!node, -1);

    sio_list_del(&node->entry);

    return 0;
}


int sio_rtpool_clean(struct sio_list_head *head)
{
    struct sio_list_head *pos;
    sio_list_foreach_del(pos, head) {
        struct sio_rtpool_node *node = NULL;
        node = (struct sio_rtpool_node *)pos;
        sio_list_del(pos);
        free(node);
        node = NULL;
    }

    return 0;
}

int sio_rtpool_destory(struct sio_rtpool *rtpool)
{
    SIO_COND_CHECK_RETURN_VAL(!rtpool,-1);

    sio_rtpool_clean(&rtpool->head);

    free(rtpool);

    return 0;
}