
#ifndef CONTEXT_H__
#define CONTEXT_H__

#include <stdint.h>

#include "internal.h"

#ifdef CTX_IMPL

#if defined(__i386__) // 32-bit
struct Ctx {
    uint32_t ebx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t eip;
};
#elif defined(__x86_64__) // 64-bit
struct Ctx {
    uint64_t rbx;
    uint64_t rsp;
    uint64_t rbp;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rip;
};
#else
#error "This architecture is not supported"
#endif /* __i386__ vs __x86_64__ */

typedef struct Ctx Ctx;

#else 

#include <ucontext.h>

typedef ucontext_t Ctx;

#endif /* CTX_IMPL */

#endif /*CONTEXT_H__ */

