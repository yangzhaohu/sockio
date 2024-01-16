#include "moudle/sio_locatmod.h"
#include <string.h>
#include <stdlib.h>
#include "sio_common.h"
#include "sio_list.h"

struct sio_locatmod_node
{
    struct sio_list_head entry;
    char name[32];
    void *ptr;
};

typedef struct sio_list_head sio_locatmod_head;

struct sio_locatmod
{
    sio_locatmod_head head;
};

static inline
struct sio_locatmod_node *sio_locatmod_find(sio_locatmod_head *head, const char *name)
{
    struct sio_locatmod_node *node = NULL;
    struct sio_list_head *pos;
    sio_list_foreach(pos, head) {
        node = (struct sio_locatmod_node *)pos;
        if (strcmp(node->name, name) == 0) {
            return node;
        }
    }

    return NULL;
}

struct sio_locatmod *sio_locatmod_create()
{
    struct sio_locatmod *locatmod = malloc(sizeof(struct sio_locatmod));
    SIO_COND_CHECK_RETURN_VAL(!locatmod, NULL);

    memset(locatmod, 0, sizeof(struct sio_locatmod));

    sio_list_init(&locatmod->head);

    return locatmod;
}

int sio_locatmod_add(struct sio_locatmod *locatmod, const char *name, void *data)
{
    if (sio_locatmod_find(&locatmod->head, name) != NULL) {
        return -1;
    }

    struct sio_locatmod_node *node = malloc(sizeof(struct sio_locatmod_node));
    SIO_COND_CHECK_RETURN_VAL(!node, -1);

    memset(node, 0, sizeof(struct sio_locatmod_node));

    int len = strlen(name);
    len = len > 31 ? 31 : len;
    memcpy(node->name, name, len);
    node->ptr = data;

    sio_list_add(&node->entry, &locatmod->head);

    return 0;
}

int sio_locatmod_remove(struct sio_locatmod *locatmod, const char *name)
{
    struct sio_locatmod_node *node = sio_locatmod_find(&locatmod->head, name);
    if (node == NULL) {
        return -1;
    }

    sio_list_del(&node->entry);

    return 0;
}

void *sio_locatmod_cat(struct sio_locatmod *locatmod, const char *name)
{
    struct sio_locatmod_node *node = sio_locatmod_find(&locatmod->head, name);
    if (node != NULL) {
        return node->ptr;
    }

    return NULL;
}

static inline
int sio_locatmod_clean_imp(struct sio_locatmod *locatmod)
{
    struct sio_locatmod_node *node = NULL;
    struct sio_list_head *pos;
    sio_list_foreach(pos, &locatmod->head) {
        node = (struct sio_locatmod_node *)pos;
        sio_list_del(pos);
        free(node);
        node = NULL;
    }

    return 0;
}

int sio_locatmod_clean(struct sio_locatmod *locatmod)
{
    return sio_locatmod_clean_imp(locatmod);
}

int sio_locatmod_destory(struct sio_locatmod *locatmod)
{
    sio_locatmod_clean_imp(locatmod);
    free(locatmod);

    return 0;
}