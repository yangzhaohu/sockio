#include "sio_rtplive.h"
#include <stdlib.h>
#include <string.h>
#include "sio_common.h"
#include "sio_list.h"
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
    node = sio_rtplive_find_imp(&rtplive->head, rtpchn);
    SIO_COND_CHECK_RETURN_VAL(node, 0);

    node = malloc(sizeof(struct sio_rtpchn_node));
    SIO_COND_CHECK_RETURN_VAL(!node, -1);

    node->rtpchn = rtpchn;
    sio_list_add(&node->entry, &rtplive->head);

    return 0;
}

int sio_rtplive_rm_senddst(sio_rtspdev_t dev, struct sio_rtpchn *rtpchn)
{
    struct sio_rtplive *rtplive = (struct sio_rtplive *)dev;

    struct sio_rtpchn_node *node = NULL;
    node = sio_rtplive_find_imp(&rtplive->head, rtpchn);
    SIO_COND_CHECK_RETURN_VAL(!node, 0);

    sio_list_del(&node->entry);

    return 0;
}

int sio_rtplive_record(sio_rtspdev_t dev, const char *data, unsigned int len)
{
    struct sio_rtplive *rtplive = (struct sio_rtplive *)dev;

    struct sio_rtpchn_node *node = NULL;
    struct sio_list_head *pos;
    sio_list_foreach(pos, &rtplive->head) {
        node = (struct sio_rtpchn_node *)pos;
        sio_rtpchn_rtpsend(node->rtpchn, data, len);
    }

    return 0;
}

int sio_rtplive_destory(struct sio_rtplive *rtplive)
{
    free(rtplive);

    return 0;
}

int sio_rtplive_close(sio_rtspdev_t dev)
{
    struct sio_rtplive *rtplive = (struct sio_rtplive *)dev;

    free(rtplive);

    return 0;
}