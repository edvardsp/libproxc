

#include "util/debug.h"
#include "internal.h"

#ifdef CTX_IMPL

/*
 * void _ctx_get(Ctx *ctx);
 */
void _ctx_get(Ctx *ctx);
__asm__(
"   .global _ctx_get           \n"
"_ctx_get:                     \n"
#if defined(__i386__)
"   movl    0x04(%%esp), %%edx \n" // ctx = %edx
"                              \n"
"   movl    %%ebx, 0x00(%%edx) \n" // %ebx
"   movl    %%esi, 0x04(%%edx) \n" // %esi
"   movl    %%edi, 0x08(%%edx) \n" // %edi
"   movl    %%ebp, 0x0C(%%edx) \n" // %ebp
"   movl    %%esp, 0x10(%%edx) \n" // %esp
"                              \n"
"   movl    (%%esp), %%eax     \n" // %eip
"   movl    %%eax, 0x14(%%edx) \n" 
"                              \n"
"   xor     %%eax, %%eax       \n"
#elif defined(__x86_64__)
"   # %rdi is ctx            \n"
"   movq    %rbx, 0x00(%rdi) \n" // %rbx
"   movq    %rsp, 0x08(%rdi) \n" // %rsp
"   movq    %rbp, 0x10(%rdi) \n" // %rbp
"   movq    %r12, 0x18(%rdi) \n" // %r12
"   movq    %r13, 0x20(%rdi) \n" // %r13
"   movq    %r14, 0x28(%rdi) \n" // %r14
"   movq    %r15, 0x30(%rdi) \n" // %r15
"                            \n"
"   movq    (%rsp), %rax     \n" // %rip
"   movq    %rax, 0x38(%rdi) \n" 
"                            \n"
"   xor     %rax, %rax       \n"
#endif
"   ret                      \n"
);

void ctx_init(Ctx *ctx, Proc *proc)
{
    _ctx_get(ctx);
    if (!proc) return;

    uintptr_t *sp = (uintptr_t *)((uintptr_t)proc->stack.ptr + proc->stack.size);
    sp -= 4;

    sp[1] = (uintptr_t)NULL; // return address
    sp[2] = (uintptr_t)proc; // arg 
#if defined(__i386__)
    ctx->eip = (uint32_t)proc_mainfxn;
    ctx->ebp = (uint32_t)(sp + 1);
    ctx->esp = (uint32_t)sp;
#elif defined(__x86_64__)
    ctx->rip = (uint64_t)proc_mainfxn;
    ctx->rbp = (uint64_t)(sp + 1);
    ctx->rsp = (uint64_t)sp;
    __asm__("movq %0, %%rdi\n" : : "r" (proc));
#endif
}

/*
 * void ctx_switch(Ctx *from, Ctx *to);
 */
#if defined(__i386__) // 32-bit
__asm__(
"   .text                       \n"
"   .p2align 2,,3               \n"
"   .global ctx_switch          \n"
"   .type ctx_switch, @function \n"
"ctx_switch:                    \n"
"   movl    0x04(%esp),    %edx \n" // from = %edx
"                               \n"
"   movl    %ebx,    0x00(%edx) \n" // %ebx
"   movl    %esi,    0x04(%edx) \n" // %esi
"   movl    %edi,    0x08(%edx) \n" // %edi
"   movl    %ebp,    0x0C(%edx) \n" // %ebp
"   movl    %esp,    0x10(%edx) \n" // %esp
"                               \n"
"   movl    (%esp),        %eax \n" // %eip
"   movl    %eax,    0x14(%edx) \n" 
"                               \n"
"   movl    0x08(%esp),  %edx   \n" // to = %edx
"                               \n"
"   movl    0x00(%edx),  %ebx   \n" // %ebx
"   movl    0x04(%edx),  %esi   \n" // %esi
"   movl    0x08(%edx),  %edi   \n" // %edi
"   movl    0x0C(%edx),  %ebp   \n" // %ebp
"   movl    0x10(%edx),  %esp   \n" // %esp
"                               \n"
"   movl    0x14(%edx),  %eax   \n" // %eip
"   movl    %eax,        (%esp) \n"
"                               \n"
"   ret                         \n"
);
#elif defined(__x86_64__) // 64-bit
__asm__(
"   .text                       \n"
"   .p2align 4,,15              \n"
"   .global ctx_switch          \n"
"   .type ctx_switch, @function \n"
"ctx_switch:                    \n"
"   movq    %rbx,    0x00(%rdi) \n" // %rbx
"   movq    %rsp,    0x08(%rdi) \n" // %rsp
"   movq    %rbp,    0x10(%rdi) \n" // %rbp
"   movq    %r12,    0x18(%rdi) \n" // %r12
"   movq    %r13,    0x20(%rdi) \n" // %r13
"   movq    %r14,    0x28(%rdi) \n" // %r14
"   movq    %r15,    0x30(%rdi) \n" // %r15
"                               \n"
"   movq    (%rsp),  %rax       \n" // %rip
"   movq    %rax,    0x38(%rdi) \n" 
"                               \n"
"   movq    0x00(%rsi),  %rbx   \n" // %rbx
"   movq    0x08(%rsi),  %rsp   \n" // %rsp
"   movq    0x10(%rsi),  %rbp   \n" // %rbp
"   movq    0x18(%rsi),  %r12   \n" // %r12
"   movq    0x20(%rsi),  %r13   \n" // %r13
"   movq    0x28(%rsi),  %r14   \n" // %r14
"   movq    0x30(%rsi),  %r15   \n" // %r15
"                               \n"
"   movq    0x38(%rsi),  %rax   \n" // %rip
"   movq    %rax,        (%rsp) \n"
"                               \n"
"   ret                         \n"
);
#endif /* __i386__ vs __x86_64__ */

#else

void ctx_init(Ctx *ctx, Proc *proc)
{
    int ret;
    ret = getcontext(ctx);
    ASSERT_0(ret);
    if (!proc) return;

    ctx->uc_link          = &proc->sched->ctx;
    ctx->uc_stack.ss_sp   = proc->stack.ptr;
    ctx->uc_stack.ss_size = proc->stack.size; 
    makecontext(ctx, (void(*)(void))proc_mainfxn, 1, proc);
    ASSERT_0(ret);
}

void ctx_switch(Ctx *from, Ctx *to)
{
    ASSERT_NOTNULL(from);
    ASSERT_NOTNULL(to);

    int ret;
    ret = swapcontext(from, to);
    ASSERT_0(ret);
}

#endif /* CTX_IMPL */

