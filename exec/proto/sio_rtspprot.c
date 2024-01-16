#include "sio_rtspprot.h"
#include <string.h>
#include <stdlib.h>
#include "sio_common.h"
#include "sio_log.h"

static const char *g_method_strings[] =
{
    [SIO_RTSP_INVALID] = "INVALID",
    [SIO_RTSP_OPTIONS] = "OPTIONS",
    [SIO_RTSP_DESCRIBE] = "DESCRIBE",
    [SIO_RTSP_SETUP] = "SETUP",
    [SIO_RTSP_PLAY] = "PLAY",
    [SIO_RTSP_PAUSE] = "PAUSE",
    [SIO_RTSP_TEARDOWN] = "TEARDOWN",
    [SIO_RTSP_ANNOUNCE] = "ANNOUNCE",
    [SIO_RTSP_GET_PARAMETER] = "GET_PARAMETER",
    [SIO_RTSP_RECORD] = "RECORD",
    [SIO_RTSP_REDIRECT] = "REDIRECT",
    [SIO_RTSP_SET_PARAMETER] = "SET_PARAMETER",
};

enum sio_rtsp_method_mask
{
    SIO_RTSP_MASK_INVALID = 1 << SIO_RTSP_INVALID,
    SIO_RTSP_MASK_OPTIONS = 1 << SIO_RTSP_OPTIONS,
    SIO_RTSP_MASK_DESCRIBE = 1 << SIO_RTSP_DESCRIBE,
    SIO_RTSP_MASK_SETUP = 1 << SIO_RTSP_SETUP,
    SIO_RTSP_MASK_PLAY = 1 << SIO_RTSP_PLAY,
    SIO_RTSP_MASK_PAUSE = 1 << SIO_RTSP_PAUSE,
    SIO_RTSP_MASK_TEARDOWN = 1 << SIO_RTSP_TEARDOWN,
    SIO_RTSP_MASK_ANNOUNCE = 1 << SIO_RTSP_ANNOUNCE,
    SIO_RTSP_MASK_GET_PARAMETER = 1 << SIO_RTSP_GET_PARAMETER,
    SIO_RTSP_MASK_RECORD = 1 << SIO_RTSP_RECORD,
    SIO_RTSP_MASK_REDIRECT = 1 << SIO_RTSP_REDIRECT,
    SIO_RTSP_MASK_SET_PARAMETER = 1 << SIO_RTSP_SET_PARAMETER
};

enum sio_rtspprot_stat
{
    SIO_RTSPPROT_STAT_BEGIN,
    SIO_RTSPPROT_STAT_METHOD,
    SIO_RTSPPROT_STAT_URL_STRAT,
    SIO_RTSPPROT_STAT_URL,
    SIO_RTSPPROT_STAT_VER_START,
    SIO_RTSPPROT_STAT_VER_R,
    SIO_RTSPPROT_STAT_VER_RT,
    SIO_RTSPPROT_STAT_VER_RTS,
    SIO_RTSPPROT_STAT_VER_RTSP,
    SIO_RTSPPROT_STAT_VER_MAJOR,
    SIO_RTSPPROT_STAT_VER_DOT,
    SIO_RTSPPROT_STAT_VER_MINOR,
    SIO_RTSPPROT_STAT_LINE_START,
    SIO_RTSPPROT_STAT_LINE_DONE,
    SIO_RTSPPROT_STAT_END_START,
    SIO_RTSPPROT_STAT_END,
    SIO_RTSPPROT_STAT_FIELD,
    SIO_RTSPPROT_STAT_VALUE_START,
    SIO_RTSPPROT_STAT_VALUE,
    SIO_RTSPPROT_STAT_VALUE_LINE_DONE,
    SIO_RTSPPROT_STAT_COMPLETE
};

struct sio_rtspprot_fsa
{
    enum sio_rtspprot_stat stat;
    const char *at;
    int inx;

    unsigned int method;
    unsigned int methmask;
    unsigned int ver;
    unsigned char major;
    unsigned char minor;
    unsigned int seq;
};

struct sio_rtspprot_owner
{
    void *private;
    struct sio_rtspprot_ops ops;
};

struct sio_rtspprot
{
    struct sio_rtspprot_fsa fsa;
    struct sio_rtspprot_owner owner;
};

#define sio_rtspprot_callcb(func, ...) \
    do { \
        if (owner->ops.func != NULL) { \
            int ret = owner->ops.func(rtspprot, ##__VA_ARGS__); \
            if (ret == -1) { \
                \
            } \
        } \
    } while (0);

struct sio_rtspprot *sio_rtspprot_create(void)
{
    struct sio_rtspprot *rtspprot = malloc(sizeof(struct sio_rtspprot));
    SIO_COND_CHECK_RETURN_VAL(!rtspprot, NULL);

    memset(rtspprot, 0, sizeof(struct sio_rtspprot));

    return rtspprot;
}

int sio_rtspprot_setopt(struct sio_rtspprot *rtspprot, enum sio_rtspprot_optcmd cmd, union sio_rtspprot_opt *opt)
{
    SIO_COND_CHECK_RETURN_VAL(!rtspprot, -1);
    struct sio_rtspprot_owner *owner = &rtspprot->owner;

    int ret = 0;
    switch (cmd) {
    case SIO_RTSPPROT_PRIVATE:
        owner->private = opt->private;
        break;

    case SIO_RTSPPROT_OPS:
        owner->ops = opt->ops;
        break;
    
    default:
        ret = -1;
        break;
    }

    return ret;
}

int sio_rtspprot_getopt(struct sio_rtspprot *rtspprot, enum sio_rtspprot_optcmd cmd, union sio_rtspprot_opt *opt)
{
    SIO_COND_CHECK_RETURN_VAL(!rtspprot, -1);
    struct sio_rtspprot_owner *owner = &rtspprot->owner;

    int ret = 0;
    switch (cmd) {
    case SIO_RTSPPROT_PRIVATE:
        opt->private = owner->private;
        break;
    
    default:
        ret = -1;
        break;
    }

    return ret;
}

int sio_rtspprot_process(struct sio_rtspprot *rtspprot, const char *data, unsigned int len)
{
    SIO_COND_CHECK_RETURN_VAL(!rtspprot, -1);

    struct sio_rtspprot_fsa *fsa = &rtspprot->fsa;
    enum sio_rtspprot_stat *stat = &fsa->stat;
    struct sio_rtspprot_owner *owner = &rtspprot->owner;

    unsigned int i = 0;
    for (; i < len; i++) {
        char ch = data[i];
        switch (*stat) {
        case SIO_RTSPPROT_STAT_BEGIN:
            fsa->method = SIO_RTSP_MASK_INVALID;
            switch (ch) {
            case 'O': fsa->method = SIO_RTSP_MASK_OPTIONS; break;
            case 'D': fsa->method = SIO_RTSP_MASK_DESCRIBE; break;
            case 'S': fsa->method = SIO_RTSP_MASK_SETUP | SIO_RTSP_MASK_SET_PARAMETER; break;
            case 'P': fsa->method = SIO_RTSP_MASK_PLAY | SIO_RTSP_MASK_PAUSE; break;
            case 'T': fsa->method = SIO_RTSP_MASK_TEARDOWN; break;
            case 'A': fsa->method = SIO_RTSP_MASK_ANNOUNCE; break;
            case 'G': fsa->method = SIO_RTSP_MASK_GET_PARAMETER; break;
            case 'R': fsa->method = SIO_RTSP_MASK_RECORD | SIO_RTSP_MASK_REDIRECT; break;
            default: break;
            }

            if (fsa->method == SIO_RTSP_MASK_INVALID) {
                return -1;
            }
            sio_rtspprot_callcb(prot_begin, fsa->at);

            fsa->at = &data[i];
            fsa->inx = 1;
            *stat = SIO_RTSPPROT_STAT_METHOD;
            break;

        case SIO_RTSPPROT_STAT_METHOD:
        {
            unsigned int bit = 0;
            unsigned int method = fsa->method;
            do {
                if ((method & 0x01) != 0) {
                    const char *match = g_method_strings[bit];
                    if (ch == ' ' && match[fsa->inx] == 0) {
                        sio_rtspprot_callcb(prot_method, fsa->at, fsa->inx);
                        // SIO_LOGE("method: %.*s,\n", fsa->inx, fsa->at);
                        *stat = SIO_RTSPPROT_STAT_URL_STRAT;
                        break;
                    } else if (ch != match[fsa->inx]) {
                        fsa->method &= ~(1 << bit);
                    }

                    if (fsa->method == SIO_RTSP_MASK_INVALID) {
                        goto error;
                    }
                }

                bit++;
                method = method >> 1;
                if (method == 0) break;
            } while (bit < 32);

            ++fsa->inx;
            break;
        }

        case SIO_RTSPPROT_STAT_URL_STRAT:
            if (ch == ' ') break;

            fsa->at = &data[i];
            fsa->inx = 1;
            *stat = SIO_RTSPPROT_STAT_URL;
            break;
        
        case SIO_RTSPPROT_STAT_URL:
            if (ch == ' ') {
                sio_rtspprot_callcb(prot_url, fsa->at, fsa->inx);
                // SIO_LOGE("url: %.*s,\n", fsa->inx, fsa->at);
                *stat = SIO_RTSPPROT_STAT_VER_START;
            }
            ++fsa->inx;
            break;

        case SIO_RTSPPROT_STAT_VER_START:
            if (ch == ' ') break;

            if (ch != 'R') goto error;
            fsa->at = &data[i];
            fsa->inx = 1;
            *stat = SIO_RTSPPROT_STAT_VER_R;
            break;
        case SIO_RTSPPROT_STAT_VER_R:
            if (ch != 'T') goto error;
            ++fsa->inx;
            *stat = SIO_RTSPPROT_STAT_VER_RT;
            break;
        case SIO_RTSPPROT_STAT_VER_RT:
            if (ch != 'S') goto error;
            ++fsa->inx;
            *stat = SIO_RTSPPROT_STAT_VER_RTS;
            break;
        case SIO_RTSPPROT_STAT_VER_RTS:
            if (ch != 'P') goto error;
            ++fsa->inx;
            *stat = SIO_RTSPPROT_STAT_VER_RTSP;
            break;
        case SIO_RTSPPROT_STAT_VER_RTSP:
            if (ch != '/') goto error;
            ++fsa->inx;
            *stat = SIO_RTSPPROT_STAT_VER_MAJOR;
            break;
        case SIO_RTSPPROT_STAT_VER_MAJOR:
            if (ch < '0' || ch > '9') goto error;
            fsa->major = ch - '0';
            ++fsa->inx;
            *stat = SIO_RTSPPROT_STAT_VER_DOT;
            break;
        case SIO_RTSPPROT_STAT_VER_DOT:
            if (ch != '.') goto error;
            ++fsa->inx;
            *stat = SIO_RTSPPROT_STAT_VER_MINOR;
            break;
        case SIO_RTSPPROT_STAT_VER_MINOR:
            if (ch < '0' || ch > '9') goto error;
            fsa->minor = ch - '0';
            ++fsa->inx;
            *stat = SIO_RTSPPROT_STAT_LINE_START;
            // SIO_LOGE("ver: %1d.%1d,\n", fsa->major, fsa->minor);
            break;

        case SIO_RTSPPROT_STAT_LINE_START:
            if (ch == '\r') {
                *stat = SIO_RTSPPROT_STAT_LINE_DONE;
                break;
            }
            if (ch == '\n') {
                *stat = SIO_RTSPPROT_STAT_END_START;
                break;
            }
            break;
        case SIO_RTSPPROT_STAT_LINE_DONE:
            if (ch != '\n') goto error;
            *stat = SIO_RTSPPROT_STAT_END_START;
            break;

        case SIO_RTSPPROT_STAT_END_START:
            if (ch == '\n') {
                *stat = SIO_RTSPPROT_STAT_COMPLETE;
                sio_rtspprot_callcb(prot_complete);
                break;
            }
            else if (ch == '\r') {
                *stat = SIO_RTSPPROT_STAT_END;
                break;
            }

            if (ch == ' ') goto error;
            fsa->at = &data[i];
            fsa->inx = 1;
            *stat = SIO_RTSPPROT_STAT_FIELD;
            break;
        case SIO_RTSPPROT_STAT_END:
            if (ch != '\n') goto error;
            *stat = SIO_RTSPPROT_STAT_COMPLETE;
            sio_rtspprot_callcb(prot_complete);
            break;

        case SIO_RTSPPROT_STAT_FIELD:
            if (ch == '\r' || ch == '\n') goto error;
            if (ch == ':') {
                sio_rtspprot_callcb(prot_filed, fsa->at, fsa->inx);
                // SIO_LOGE("field: %.*s,\n", fsa->inx, fsa->at);
                *stat = SIO_RTSPPROT_STAT_VALUE_START;
            }
            ++fsa->inx;
            break;
        case SIO_RTSPPROT_STAT_VALUE_START:
            if (ch == ' ') break;
            if (ch == '\r') {
                *stat = SIO_RTSPPROT_STAT_VALUE_LINE_DONE;
            } else if (ch == '\n') {
                *stat = SIO_RTSPPROT_STAT_END_START;
            }
            fsa->at = &data[i];
            fsa->inx = 1;
            *stat = SIO_RTSPPROT_STAT_VALUE;
            break;
        case SIO_RTSPPROT_STAT_VALUE:
            if (ch == '\r') {
                // SIO_LOGE("value: %.*s,\n", fsa->inx, fsa->at);
                sio_rtspprot_callcb(prot_value, fsa->at, fsa->inx);
                *stat = SIO_RTSPPROT_STAT_VALUE_LINE_DONE;
            } else if (ch == '\n') {
                // SIO_LOGE("value: %.*s,\n", fsa->inx, fsa->at);
                sio_rtspprot_callcb(prot_value, fsa->at, fsa->inx);
                *stat = SIO_RTSPPROT_STAT_END_START;
            }
            ++fsa->inx;
            break;

        case SIO_RTSPPROT_STAT_VALUE_LINE_DONE:
            if (ch != '\n') goto error;
            *stat = SIO_RTSPPROT_STAT_END_START;
            break;

        case SIO_RTSPPROT_STAT_COMPLETE:
            return i;
            break;
        default:
            goto error;
            break;
        }
    }

error:
    *stat = *stat == SIO_RTSPPROT_STAT_COMPLETE ? SIO_RTSPPROT_STAT_BEGIN : *stat;
    return i;
}

int sio_rtspprot_method(struct sio_rtspprot *rtspprot)
{
    SIO_COND_CHECK_RETURN_VAL(!rtspprot, -1);

    struct sio_rtspprot_fsa *fsa = &rtspprot->fsa;

    return fsa->method;
}

int sio_rtspprot_destory(struct sio_rtspprot *rtspprot)
{
    SIO_COND_CHECK_RETURN_VAL(!rtspprot, -1);
    free(rtspprot);

    return 0;
}