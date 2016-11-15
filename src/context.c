
#include <sys/mman.h>

#include "internal.h"

#ifdef CTX_IMPL

/*
 * void _ctx_get(Ctx *ctx);
 */
void _ctx_get(Ctx *ctx);
__asm__(
"   .global _ctx_get         \n"
"_ctx_get:                   \n"
#if defined(__i386__)
"   movl    0x04(%esp), %edx \n" // ctx = %edx
"                            \n"
"   movl    %ebx, 0x00(%edx) \n" // %ebx
"   movl    %esi, 0x04(%edx) \n" // %esi
"   movl    %edi, 0x08(%edx) \n" // %edi
"   movl    %ebp, 0x0C(%edx) \n" // %ebp
"   movl    %esp, 0x10(%edx) \n" // %esp
"                            \n"
"   movl    (%esp), %eax     \n" // %eip
"   movl    %eax, 0x14(%edx) \n" 
"                            \n"
"   xor     %eax, %eax       \n"
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

void _trampoline(void)
{
    Proc *proc = proc_self();
    proc->state = PROC_ENDED;
    proc_yield(proc);
}

void ctx_init(Ctx *ctx, Proc *proc)
{
    _ctx_get(ctx);
    if (!proc) return;

    // set stackpointer (sp) to top+1 of stack
    volatile uintptr_t *sp = (uintptr_t *)((uintptr_t)proc->stack.ptr + proc->stack.size);
    // align stack
    sp = (uintptr_t *)((uintptr_t)sp & (uintptr_t)~0xf); 

    sp -= 4;
    sp[1] = (uintptr_t)_trampoline; // return address
#if defined(__i386__)
    sp[2]    = (uintptr_t)proc;
    ctx->eip = (uint32_t)proc_mainfxn;
    ctx->ebp = (uint32_t)(sp + 1);
    ctx->esp = (uint32_t)sp;
#elif defined(__x86_64__)
    ctx->rdi = (uint64_t)proc;
    ctx->rip = (uint64_t)proc_mainfxn;
    ctx->rbp = (uint64_t)(sp + 1);
    ctx->rsp = (uint64_t)sp;
#endif
}

/*
 * void ctx_switch(Ctx *from, Ctx *to);
 */
__asm__(
//"   .text                       \n"
//"   .p2align 2,,3               \n"
"   .global ctx_switch          \n"
//"   .type ctx_switch, @function \n"
"ctx_switch:                    \n"
#if defined(__i386__) // 32-bit
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
#elif defined(__x86_64__) // 64-bit
//"   .text                       \n"
//"   .p2align 4,,15              \n"
//"   .global ctx_switch          \n"
//"   .type ctx_switch, @function \n"
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
"   movq    0x40(%rsi),  %rdi   \n" // setup arg1 for proc_mainfxn
"                               \n"
"   movq    0x38(%rsi),  %rax   \n" // %rip
"   movq    %rax,        (%rsp) \n"
#endif /* __i386__ vs __x86_64__ */
"   ret                         \n"
);


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

void ctx_madvise(Proc *proc)
{
    ASSERT_NOTNULL(proc);

    Ctx *ctx = &proc->ctx;
    intptr_t sp;

#if   defined(CTX_IMPL) && defined(__i386__) // 32-bit
    sp = (intptr_t)ctx->esp;
#elif defined(CTX_IMPL) && defined(__x86_64__) // 64-bit
    sp = (intptr_t)ctx->rsp;
#elif defined(__i386__) // 32-bit
    sp = (intptr_t)ctx->uc_mcontext.gregs[REG_ESP];
#elif defined(__x86_64__) // 64-bit
    sp = (intptr_t)ctx->uc_mcontext.gregs[REG_RSP]; 
#endif 

    if (UNLIKELY(sp < (intptr_t)proc->stack.ptr)) {
        PANIC("Stack overflow\n");
    }

    size_t unused_stack = (size_t)(sp - (intptr_t)proc->stack.ptr);
    size_t used_stack = proc->stack.size - unused_stack;
    size_t page_size = proc->sched->page_size;

    #define page_floor(x) ((x) & ~(page_size - 1))
    if (page_floor(used_stack) < page_floor(proc->stack.used)) {
        int ret = madvise(proc->stack.ptr, page_floor(unused_stack), MADV_DONTNEED);
        ASSERT_0(ret);
        PANIC("yes\n");
    }

    proc->stack.used = used_stack;
}
