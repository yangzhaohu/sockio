#ifndef SIO_COMMON_H_
#define SIO_COMMON_H_

#define PRINT_MACRO_HELPER(x) #x
#define PRINT_MACRO(x) #x"="PRINT_MACRO_HELPER(x)

// #pragma message(PRINT_MACRO(var))

#define SIO_OFFSET_OF(type, member) ((unsigned long)&(((type*)0)->member))
#define SIO_CONTAINER_OF(ptr, type, member)	((type *)((char *)(ptr) - SIO_OFFSET_OF(type, member)))

#define c(cond)        \
    if (cond) {                                 \
        return;                                 \
    }

#define SIO_COND_CHECK_RETURN_VAL(cond, val)    \
    if (cond) {                                 \
        return val;                             \
    }

#define SIO_COND_CHECK_CALLOPS_RETURN_NONE(cond, ...)               \
    if (cond)                                                       \
    {                                                               \
        (__VA_ARGS__);                                              \
        return;                                                     \
    }

#define SIO_COND_CHECK_CALLOPS_RETURN_VAL(cond, val, ...)           \
    if (cond)                                                       \
    {                                                               \
        (__VA_ARGS__);                                              \
        return val;                                                 \
    }


#endif