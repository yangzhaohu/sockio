#include "moudle/sio_locate.h"
#include <string.h>
#include <stdlib.h>
#include "sio_common.h"
#include "sio_list.h"
#include "pcre2/pcre2posix.h"
#include "sio_log.h"

struct sio_locate_node
{
    struct sio_list_head entry;
    struct sio_location location;
};

typedef struct sio_list_head sio_locate_head;

struct sio_locate
{
    sio_locate_head head;
};

static inline
int sio_httpmod_regex_match(const char *pattern, const char *string, int length)
{
    regex_t regex;
    int ret = pcre2_regcomp(&regex, pattern, REG_EXTENDED);
    if (ret) {
        SIO_LOGE("pcre2 compiler regex failed\n");
        return -1;
    }

    char *modify = (char *)string;
    char tmp = modify[length];
    modify[length] = 0;
    regmatch_t match[64] = { 0 };
    ret = pcre2_regexec(&regex, string, 64, match, 0);
    modify[length] = tmp;

    int matchpos = -1;
    if (ret == 0) {
        for (int i = 0; i < 64 && match[i].rm_so != -1; i++) {
            matchpos = match[i].rm_eo;
        }
    }

    pcre2_regfree(&regex);

    return matchpos;
}

static inline
struct sio_locate_node *sio_locate_find(sio_locate_head *head, const char *name)
{
    struct sio_locate_node *node = NULL;
    struct sio_list_head *pos;
    sio_list_foreach(pos, head) {
        node = (struct sio_locate_node *)pos;
        struct sio_location *location = &node->location;
        if (strcmp(location->loname, name) == 0) {
            return node;
        }
    }

    return NULL;
}

static inline
struct sio_location *sio_locate_find_location(sio_locate_head *head, const char *name)
{
    struct sio_locate_node *node = sio_locate_find(head, name);
    if (node == NULL) {
        return NULL;
    }

    struct sio_location *location = &node->location;
    if (location->type == SIO_LOCATE_HTTP_LOCATE) {
        return sio_locate_find_location(head, location->route);
    }

    return location;
}

struct sio_locate *sio_locate_create()
{
    struct sio_locate *locate = malloc(sizeof(struct sio_locate));
    SIO_COND_CHECK_RETURN_VAL(!locate, NULL);

    memset(locate, 0, sizeof(struct sio_locate));

    sio_list_init(&locate->head);

    return locate;
}

static inline
struct sio_locate_node *sio_locate_locations_create()
{
    struct sio_locate_node *node = malloc(sizeof(struct sio_locate_node));
    SIO_COND_CHECK_RETURN_VAL(!node, NULL);

    memset(node, 0, sizeof(struct sio_locate_node));

    return node;
}

static inline
int sio_locate_add_for_locate(struct sio_locate *locate, const struct sio_location *location)
{
    struct sio_location *lo = sio_locate_find_location(&locate->head, location->route);
    if (lo == NULL) {
        SIO_LOGE("not found final location\n");
        return -1;
    }

    struct sio_locate_node *node = sio_locate_locations_create();
    SIO_COND_CHECK_RETURN_VAL(!node, -1);

    node->location.type = SIO_LOCATE_HTTP_DIRECT;
    memcpy(node->location.loname, location->loname, strlen(location->loname));
    memcpy(node->location.regex, location->regex, strlen(location->regex));
    memcpy(node->location.route, lo->route, strlen(lo->route));

    sio_list_add(&node->entry, &locate->head);

    SIO_LOGE("---> final: loname: %s, regex: %s, route: %s\n\n", 
        node->location.loname, node->location.regex, node->location.route);

    return 0;
}

int sio_locate_add(struct sio_locate *locate, const struct sio_location *location)
{
    SIO_LOGE("location type: %d, loname: %s, regex: %s, route: %s\n", 
        location->type, location->loname, location->regex, location->route);
    if (location->type == SIO_LOCATE_HTTP_LOCATE) {
        return sio_locate_add_for_locate(locate, location);
    }

    struct sio_locate_node *node = sio_locate_locations_create();
    SIO_COND_CHECK_RETURN_VAL(!node, -1);

    memcpy(&node->location, location, sizeof(struct sio_location));
    sio_list_add(&node->entry, &locate->head);

    SIO_LOGE("---> final: loname: %s, regex: %s, route: %s\n\n", 
        location->loname, location->regex, location->route);

    return 0;
}

int sio_locate_remove(struct sio_locate *locate, const char *name)
{
    struct sio_locate_node *node = sio_locate_find(&locate->head, name);
    if (node == NULL) {
        return -1;
    }

    sio_list_del(&node->entry);

    return -1;
}

int sio_locate_match(struct sio_locate *locate, const char *str, int len, struct sio_location *location)
{
    int matchpos = -1;
    struct sio_locate_node *node = NULL;
    struct sio_list_head *pos;
    sio_list_foreach(pos, &locate->head) {
        node = (struct sio_locate_node *)pos;
        matchpos = sio_httpmod_regex_match(node->location.regex, str, len);
        if (matchpos != -1) {
            memcpy(location, &node->location, sizeof(struct sio_location));
            break;
        }
    }

    return matchpos;
}

int sio_locate_clean_imp(struct sio_locate *locate)
{
    struct sio_locate_node *node = NULL;
    struct sio_list_head *pos;
    sio_list_foreach_del(pos, &locate->head) {
        node = (struct sio_locate_node *)pos;
        SIO_LOGI("del: %s\n", node->location.loname);
        sio_list_del(pos);
        free(node);
        node = NULL;
    }

    return 0;
}

int sio_locate_clean(struct sio_locate *locate)
{
    return sio_locate_clean_imp(locate);
}

int sio_locate_destory(struct sio_locate *locate)
{
    sio_locate_clean_imp(locate);
    free(locate);

    return 0;
}
