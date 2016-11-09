
#ifndef DEBUG_H__
#define DEBUG_H__

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#if defined(NDEBUG)

#   define ASSERT_0(x)           (void)(x)
#   define ASSERT_TRUE(x)        (void)(x)
#   define ASSERT_FALSE(x)       (void)(x)
#   define ASSERT_NOTNULL(x)     (void)(x)
#   define ASSERT_EQ(lhs, rhs)   do { (void)(lhs); (void)(rhs) } while (0)
#   define ASSERT_NEQ(lhs, rhs)  do { (void)(lhs); (void)(rhs) } while (0)

#   define PDEBUG(...)  /* do nothing */
#   define PERROR(msg)  /* do nothing */

#   define PANIC(...)  panic()

#else /* !defined(NDEBUG) */

#   define ASSERT_0(x)           assert((x) == 0)
#   define ASSERT_TRUE(x)        assert(!!(x))
#   define ASSERT_FALSE(x)       assert(!(x))
#   define ASSERT_NOTNULL(x)     assert((x) != NULL)
#   define ASSERT_EQ(lhs, rhs)   assert((x) == (val))
#   define ASSERT_NEQ(lhs, rhs)  assert((x) != (val))

#   define CYAN(txt)  "\x1b[36;1m" txt "\x1b[0m"
#   define RED(txt)   "\x1b[31;1m" txt "\x1b[0m"

#   define PDEBUG(msg, ...) \
        fprintf(stderr, "[" CYAN("DEBUG") "] %s:%d:%s(): " msg, \
                __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#   define PERROR(msg) do { \
        fprintf(stderr, "[" RED("ERROR") "] %s:%d:%s(): " msg, \
                __FILE__, __LINE__, __func__); \
        perror("\t-"); \
} while (0)

#   define PANIC(...) do { \
        PDEBUG(__VA_ARGS__); \
        abort(); \
} while (0)

#endif /* defined(NDEBUG) */

#endif /* DEBUG_H__ */

