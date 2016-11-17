
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ucontext.h>

#include "internal.h"

void proc_mainfxn(Proc *proc)
{
    ASSERT_NOTNULL(proc);
    ASSERT_NOTNULL(proc->fxn);

    /* do scheduler stuff */
    /* call ctx->fxn() */
    /* when fxn returns, do */
        /* maybe clean up context? */
        /* remove context from scheduler */
        /* return control to scheduler */

    proc->fxn();

    PDEBUG("proc_mainfxn done\n");

    proc->state = PROC_ENDED;
    proc_yield(proc);
}

Proc* proc_self(void)
{
    Proc *proc = scheduler_self()->curr_proc;
    ASSERT_NOTNULL(proc);
    return proc;
}

int proc_create(Proc **new_proc, ProcFxn fxn)
{
    ASSERT_NOTNULL(new_proc);
    ASSERT_NOTNULL(fxn);

    Proc *proc;
    if (!(proc = malloc(sizeof(Proc)))) {
        PERROR("malloc failed for Proc\n");
        return errno;
    }

    Scheduler *sched = scheduler_self();
    if (posix_memalign(&proc->stack.ptr, sched->page_size, sched->stack_size)) {
        free(proc);
        PERROR("posix_memalign failed\n");
        return errno;
    }

    /* set fxn and args */
    proc->fxn      = fxn;
    proc->args.num = 0;
    proc->args.ptr = NULL;

    /* configure members */
    proc->stack.size = sched->stack_size;
    proc->stack.used = 0;
    proc->state      = PROC_READY;
    proc->sleep_us   = 0;
    proc->sched      = sched;
    proc->proc_build = NULL;

    /* configure context */
    ctx_init(&proc->ctx, proc);

    /* register in sched totalQ */
    TAILQ_INSERT_TAIL(&sched->totalQ, proc, schedQ_node);

    *new_proc = proc;

    return 0;
}

void proc_free(Proc *proc)
{
    if (!proc) return;

    /* remove from totalQ */
    Scheduler *sched = proc->sched;
    TAILQ_REMOVE(&sched->totalQ, proc, schedQ_node);

    /* resolve ProcBuild */
    ProcBuild *build = proc->proc_build;
    if (build != NULL) {
        csp_parsebuild((Builder *)build);
    }

    free(proc->args.ptr);
    free(proc->stack.ptr);
    free(proc);
}

static inline
int _proc_copyargs(Proc *proc, void **buf, size_t buf_size)
{
    size_t new_num = proc->args.num + buf_size;
    void *ptr;
    if (!(ptr = realloc(proc->args.ptr, sizeof(void *) * new_num))) {
        free(proc);
        PERROR("realloc failed for Proc Args\n");
        return errno;
    }
    proc->args.ptr = ptr;
    memcpy(proc->args.ptr + proc->args.num, buf, sizeof(void *) * buf_size);
    proc->args.num += buf_size;
    return 0;
}

int proc_setargs(Proc *proc, va_list args)
{
    enum { ARGS_N = 32 };
    void *tmp_args[ARGS_N];
    size_t curr_n = 0;
    int ret;

    void *xarg = va_arg(args, void *);
    while (xarg != PROXC_NULL) {
        tmp_args[curr_n++] = xarg;
        xarg = va_arg(args, void *);

        /* if tmp_args is full, realloc and copy over into args */
        if (curr_n == ARGS_N) {
            ret = _proc_copyargs(proc, tmp_args, ARGS_N);
            ASSERT_0(ret);
            curr_n = 0;
        }
    } 

    if (curr_n > 0) {
        ret = _proc_copyargs(proc, tmp_args, curr_n);
        ASSERT_0(ret);
    }

    return 0;
}

void proc_yield(Proc *proc)
{
    Scheduler *sched = (!proc)
                     ? scheduler_self()
                     : proc->sched;

    PDEBUG("yielding\n");
    ctx_switch(&sched->curr_proc->ctx, &sched->ctx);
}

