
#ifndef CONTEXT_H__
#define CONTEXT_H__

#include <stdint.h>

#include "internal.h"

#ifdef CTX_IMPL

#if !defined(__i386__) && !defined(__x86_64__)
#error "Only i386 and x86_64 architecture is supported"
#endif

struct Ctx {
#if defined(__i386__) // 32-bit
    uint32_t ebx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t eip;
#elif defined(__x86_64__) // 64-bit
    uint64_t rbx;
    uint64_t rsp;
    uint64_t rbp;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rip;
    uint64_t rdi; // this is for arg1 in proc_mainfxn
#endif /* __i386__ vs __x86_64__ */
};

typedef struct Ctx Ctx;

#else /* !CTX_IMPL */

#include <ucontext.h>

typedef ucontext_t Ctx;

#endif /* CTX_IMPL */

#endif /*CONTEXT_H__ */

