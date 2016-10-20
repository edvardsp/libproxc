
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ucontext.h>

#include "debug.h"
#include "internal.h"

static
void _proc_mainfxn(Proc *proc)
{
    ASSERT_NOTNULL(proc);
    ASSERT_NOTNULL(proc->fxn);
    /* do scheduler stuff */
    /* call ctx->fxn() */
    /* when fxn returns, do */
        /* maybe clean up context? */
        /* remove context from scheduler */
        /* return control to scheduler */

    proc->fxn(proc->arg);

    proc->state = PROC_ENDED;

    PDEBUG("_proc_mainfxn done\n");
}

int proc_create(Proc **new_proc, ProcFxn fxn, void *arg)
{
    ASSERT_NOTNULL(new_proc);
    ASSERT_NOTNULL(fxn);

    Proc *proc;
    if ((proc = malloc(sizeof(Proc))) == NULL) {
        PERROR("malloc failed for Proc\n");
        return errno;
    }
    memset(proc, 0, sizeof(Proc));

    Scheduler *sched = scheduler_self();
    if ((proc->stack = malloc(sched->stack_size)) == NULL) {
        free(proc);
        PERROR("malloc failed for Proc stack\n");
        return errno;
    }
    //memset(proc->stack, 0, sched->stack_size);

    /* if (posix_memalign(&proc->stack, (size_t)getpagesize(), sched->stack_size)) { */
    /*     free(proc); */
    /*     PERROR("posix_memalign failed\n"); */
    /*     return errno; */
    /* } */

    proc->fxn = fxn;
    proc->arg = arg;
    proc->stack_size = sched->stack_size;
    proc->state = PROC_READY;
    proc->sched = sched;

    /* configure context */
    ASSERT_0(getcontext(&proc->ctx));
    proc->ctx.uc_link          = &sched->ctx;
    proc->ctx.uc_stack.ss_sp   = proc->stack;
    proc->ctx.uc_stack.ss_size = proc->stack_size;
    makecontext(&proc->ctx, (void(*)(void))_proc_mainfxn, 1, proc);

    *new_proc = proc;

    return 0;
}

void proc_free(Proc *proc)
{
    if (proc == NULL) return;

    free(proc->stack);
    memset(proc, 0, sizeof(Proc));
    free(proc);
}

void proc_yield(void)
{
    Scheduler *sched = scheduler_self();

    sched->curr_proc->state = PROC_READY;

    PDEBUG("yielding\n");
    ASSERT_0(scheduler_switch(&sched->curr_proc->ctx, &sched->ctx));
}

