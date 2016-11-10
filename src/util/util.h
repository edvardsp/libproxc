
#ifndef UTIL_H__
#define UTIL_H__

#if defined(__GNUC__) || defined(__llvm__)

#   define LIKELY(x)    __builtin_expect(!!(x), 1)
#   define UNLIKELY(x)  __builtin_expect(!!(x), 0)

#else

#   define LIKELY(x)    (x)
#   define UNLIKELY(x)  (x)

#endif /* defined(__GNUC__) || defined(__llvm__) */

#endif /* UTIL_H__ */
