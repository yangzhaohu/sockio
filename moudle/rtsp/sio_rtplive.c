#include "sio_rtplive.h"
#include <stdlib.h>
#include <string.h>
#include "sio_common.h"
#include "sio_list.h"
#include "sio_mutex.h"
#include "sio_rtpchn.h"
#include "sio_log.h"

struct sio_rtpchn_node
{
    struct sio_list_head entry;
    struct sio_rtpchn *rtpchn;
};

typedef struct sio_list_head sio_rtpool_head;

struct sio_rtplive
{
    char descibe[4096];
    sio_rtpool_head head;
    sio_mutex mutex;
};

static inline
struct sio_rtpchn_node *sio_rtplive_find_imp(struct sio_list_head *head, struct sio_rtpchn *rtpchn)
{
    struct sio_rtpchn_node *node = NULL;
    struct sio_list_head *pos;
    sio_list_foreach(pos, head) {
        node = (struct sio_rtpchn_node *)pos;
        if (node->rtpchn == rtpchn) {
            return node;
        }
    }

    return NULL;
}

static inline
struct sio_rtplive *sio_rtplive_create()
{
    struct sio_rtplive *rtplive = malloc(sizeof(struct sio_rtplive));
    SIO_COND_CHECK_RETURN_VAL(!rtplive, NULL);

    memset(rtplive, 0, sizeof(struct sio_rtplive));

    sio_list_init(&rtplive->head);
    sio_mutex_init(&rtplive->mutex);

    return rtplive;
}

sio_rtspdev_t sio_rtplive_open(const char *name)
{
    struct sio_rtplive *rtplive = sio_rtplive_create();
    SIO_COND_CHECK_RETURN_VAL(!rtplive, NULL);

    return (sio_rtspdev_t)rtplive;
}

int sio_rtplive_start(struct sio_rtplive *rtplive)
{
    return 0;
}

int sio_rtplive_set_describe(sio_rtspdev_t dev, const char *describe, int len)
{
    SIO_COND_CHECK_RETURN_VAL(len > 4095, -1);

    struct sio_rtplive *rtplive = (struct sio_rtplive *)dev;
    memcpy(rtplive->descibe, describe, len);

    return 0;
}

int sio_rtplive_get_describe(sio_rtspdev_t dev, const char **describe)
{
    struct sio_rtplive *rtplive = (struct sio_rtplive *)dev;
    *describe = rtplive->descibe;

    return 0;
}

int sio_rtplive_add_senddst(sio_rtspdev_t dev, struct sio_rtpchn *rtpchn)
{
    struct sio_rtplive *rtplive = (struct sio_rtplive *)dev;
    struct sio_rtpchn_node *node = NULL;

    sio_mutex_lock(&rtplive->mutex);
    node = sio_rtplive_find_imp(&rtplive->head, rtpchn);
    if (node == NULL) {
        node = malloc(sizeof(struct sio_rtpchn_node));
        SIO_COND_CHECK_CALLOPS_RETURN_VAL(!node, -1,
            sio_mutex_unlock(&rtplive->mutex));

        node->rtpchn = rtpchn;
        sio_list_add(&node->entry, &rtplive->head);
    }
    sio_mutex_unlock(&rtplive->mutex);

    return 0;
}

int sio_rtplive_rm_senddst(sio_rtspdev_t dev, struct sio_rtpchn *rtpchn)
{
    struct sio_rtplive *rtplive = (struct sio_rtplive *)dev;
    struct sio_rtpchn_node *node = NULL;

    sio_mutex_lock(&rtplive->mutex);
    node = sio_rtplive_find_imp(&rtplive->head, rtpchn);
    if (node) {
        sio_list_del(&node->entry);
    }
    sio_mutex_unlock(&rtplive->mutex);

    return 0;
}

int sio_rtplive_record(sio_rtspdev_t dev, const char *data, unsigned int len)
{
    struct sio_rtplive *rtplive = (struct sio_rtplive *)dev;

    struct sio_rtpchn_node *node = NULL;
    struct sio_list_head *pos;

    sio_mutex_lock(&rtplive->mutex);
    sio_list_foreach(pos, &rtplive->head) {
        node = (struct sio_rtpchn_node *)pos;
        sio_rtpchn_rtpsend(node->rtpchn, data, len);
    }
    sio_mutex_unlock(&rtplive->mutex);

    return 0;
}

static inline
int sio_rtplive_destory(struct sio_rtplive *rtplive)
{
    struct sio_list_head *pos;
    sio_list_foreach_del(pos, &rtplive->head) {
        struct sio_rtpchn_node *node = NULL;
        node = (struct sio_rtpchn_node *)pos;
        sio_list_del(pos);
        free(node);
        node = NULL;
    }
    sio_mutex_destory(&rtplive->mutex);
    free(rtplive);

    return 0;
}

int sio_rtplive_close(sio_rtspdev_t dev)
{
    struct sio_rtplive *rtplive = (struct sio_rtplive *)dev;

    return sio_rtplive_destory(rtplive);
}