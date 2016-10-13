
#ifndef DEBUG_H__
#define DEBUG_H__

#include <stdio.h>
#include <assert.h>

#if defined(NDEBUG)

#   define ASSERT_0(x)        (void)(x)
#   define ASSERT_1(x)        (void)(x)
#   define ASSERT_NOTNULL(x)  (void)(x)
#   define ASSERT_IS(x, val)  (void)(x)

#   define PDEBUG(msg)  /* do nothing */
#   define PERROR(msg)  /* do nothing */

#else /* !defined(NDEBUG) */

#   define ASSERT_0(x)        assert(!(x))
#   define ASSERT_1(x)        assert(!!(x))
#   define ASSERT_NOTNULL(x)  assert((x) != NULL)
#   define ASSERT_IS(x, val)  assert((x) == (val))

#   define _CYAN(txt)  "\x1b[36;1m" txt "\x1b[0m"
#   define _RED(txt)   "\x1b[31;1m" txt "\x1b[0m"

#   define PDEBUG(msg)  fprintf(stderr, "[" _CYAN("DEBUG") "] %s:%d:%s(): " msg, __FILE__, __LINE__, __func__)
#   define PERROR(msg)  fprintf(stderr, "[" _RED("ERROR") "] %s:%d:%s(): " msg, __FILE__, __LINE__, __func__); perror("\t-");

#endif /* defined(NDEBUG) */

#endif /* DEBUG_H__ */

