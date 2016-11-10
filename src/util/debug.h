
#ifndef DEBUG_H__
#define DEBUG_H__

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#   define ASSERT_0(x)           ASSERT((x) == 0)  
#   define ASSERT_TRUE(x)        ASSERT(!!(x)) 
#   define ASSERT_FALSE(x)       ASSERT(!(x)) 
#   define ASSERT_NOTNULL(x)     ASSERT((x) != NULL) 
#   define ASSERT_EQ(lhs, rhs)   ASSERT((lhs) == (rhs)) 
#   define ASSERT_NEQ(lhs, rhs)  ASSERT((lhs) != (rhs))

#if defined(NDEBUG)

#   define ASSERT(x) (void)(x)

#   define PDEBUG(...)  /* do nothing */
#   define PERROR(msg)  /* do nothing */

#   define PANIC(...)  abort()

#else /* !defined(NDEBUG) */

#   define ASSERT(x) do { \
        if (!(x)) { \
            fprintf(stderr, "((" RED("ASSERT") ")) %s:%d:%s(): failed :: `" #x "`\n", \
                    __FILE__, __LINE__, __func__); \
            fflush(stderr); \
            abort(); \
        } \
} while (0)

#   define CYAN(txt)  "\x1b[36;1m" txt "\x1b[0m"
#   define RED(txt)   "\x1b[31;1m" txt "\x1b[0m"

#   define PDEBUG(msg, ...) do { \
        fprintf(stderr, "[" CYAN("DEBUG") "] %s:%d:%s(): " msg, \
                __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        fflush(stderr); \
} while (0)

#   define PERROR(msg) do { \
        fprintf(stderr, "[" RED("ERROR") "] %s:%d:%s(): " msg, \
                __FILE__, __LINE__, __func__); \
        fflush(stderr); \
        perror("\t-"); \
} while (0)

#   define PANIC(msg, ...) do { \
        fprintf(stderr, "<<" RED("PANIC") ">> %s:%d:%s(): " msg, \
                __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        fflush(stderr); \
        abort(); \
} while (0)

#endif /* defined(NDEBUG) */

#endif /* DEBUG_H__ */

