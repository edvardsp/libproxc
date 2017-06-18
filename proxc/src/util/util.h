
#ifndef UTIL_H__
#define UTIL_H__

#include <stddef.h>
#include <stdint.h>
#include <sys/time.h>

static inline
uint64_t gettimestamp(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * (uint64_t)1000000 + (uint64_t)tv.tv_usec;
}

#if defined(__GNUC__) || defined(__llvm__)

#   define LIKELY(x)    __builtin_expect(!!(x), 1)
#   define UNLIKELY(x)  __builtin_expect(!!(x), 0)

#else

#   define LIKELY(x)    (x)
#   define UNLIKELY(x)  (x)

#endif /* defined(__GNUC__) || defined(__llvm__) */

#endif /* UTIL_H__ */
