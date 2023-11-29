#ifndef SIO_COMMON_H_
#define SIO_COMMON_H_

#include "sio_def.h"

#define PRINT_MACRO_HELPER(x) #x
#define PRINT_MACRO(x) #x"="PRINT_MACRO_HELPER(x)

// str macro

#define SIO_TO_STR(a) _SIO_TO_STR(a)
#define _SIO_TO_STR(a) #a
#define SIO_CAT_STR(a, b) _SIO_CAT_STR(a, b)
#define _SIO_CAT_STR(a, b) a##b

// #pragma message(PRINT_MACRO(var))

#define SIO_MIN(a, b) ((a < b) ? a : b)
#define SIO_MAX(a, b) ((a > b) ? a : b)

#define SIO_OFFSET_OF(type, member) ((sio_uptr_t)&(((type*)0)->member))
#define SIO_CONTAINER_OF(ptr, type, member)	((type *)((char *)(ptr) - SIO_OFFSET_OF(type, member)))

#define SIO_COND_CHECK_CONTINUE(cond)           \
    if (cond) {                                 \
        continue;                               \
    }

#define SIO_COND_CHECK_CALLOPS_CONTINUE(cond, ...)          \
    if (cond) {                                             \
        (__VA_ARGS__);                                      \
        continue;                                           \
    }

#define SIO_COND_CHECK_BREAK(cond)              \
    if (cond) {                                 \
        break;                                  \
    }

#define SIO_COND_CHECK_RETURN_NONE(cond)        \
    if (cond) {                                 \
        return;                                 \
    }

#define SIO_COND_CHECK_RETURN_VAL(cond, val)    \
    if (cond) {                                 \
        return val;                             \
    }


#define SIO_COND_CHECK_CALLOPS(cond, ...)                           \
    if (cond) {                                                     \
        (__VA_ARGS__);                                              \
    }

#define SIO_COND_CHECK_CALLOPS_RETURN_NONE(cond, ...)               \
    if (cond) {                                                     \
        (__VA_ARGS__);                                              \
        return;                                                     \
    }

#define SIO_COND_CHECK_CALLOPS_RETURN_VAL(cond, val, ...)           \
    if (cond) {                                                     \
        (__VA_ARGS__);                                              \
        return val;                                                 \
    }

#define SIO_COND_CHECK_CALLOPS_BREAK(cond, ...)                     \
    if (cond) {                                                     \
        (__VA_ARGS__);                                              \
        break;                                                     \
    }

#endif