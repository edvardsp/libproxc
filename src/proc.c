
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ucontext.h>

#include "util/debug.h"
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

    proc->fxn();

    proc->state = PROC_ENDED;

    PDEBUG("_proc_mainfxn done\n");
}

int proc_create(Proc **new_proc, ProcFxn fxn)
{
    ASSERT_NOTNULL(new_proc);
    ASSERT_NOTNULL(fxn);
    /* arg can be NULL */

    Proc *proc;
    if ((proc = malloc(sizeof(Proc))) == NULL) {
        PERROR("malloc failed for Proc\n");
        return errno;
    }
    memset(proc, 0, sizeof(Proc));

    Scheduler *sched = scheduler_self();
    if ((proc->stack.ptr = malloc(sched->stack_size)) == NULL) {
        free(proc);
        PERROR("malloc failed for Proc stack\n");
        return errno;
    }

    /* if (posix_memalign(&proc->stack, (size_t)getpagesize(), sched->stack_size)) { */
    /*     free(proc); */
    /*     PERROR("posix_memalign failed\n"); */
    /*     return errno; */
    /* } */

    /* set fxn and args */
    proc->fxn = fxn;
    proc->num_args = 0;
    proc->args = NULL;

    /* configure members */
    proc->stack.size = sched->stack_size;
    proc->state = PROC_READY;
    proc->sched = sched;
    proc->par_struct = NULL;

    /* configure context */
    ASSERT_0(getcontext(&proc->ctx));
    proc->ctx.uc_link          = &sched->ctx;
    proc->ctx.uc_stack.ss_sp   = proc->stack.ptr;
    proc->ctx.uc_stack.ss_size = proc->stack.size;
    makecontext(&proc->ctx, (void(*)(void))_proc_mainfxn, 1, proc);

    *new_proc = proc;

    return 0;
}

void proc_free(Proc *proc)
{
    if (proc == NULL) return;

    free(proc->args);
    free(proc->stack.ptr);
    free(proc);
}

int proc_setargs(Proc *proc, va_list args)
{
    /* FIXME for over 16 args */
    #define SMALL_SIZE_OPT 16
    void *tmp_args[SMALL_SIZE_OPT]; 
    void *xarg = va_arg(args, void *);
    while (xarg != NULL) {
        ASSERT_TRUE(proc->num_args < SMALL_SIZE_OPT);
        tmp_args[proc->num_args++] = xarg;
        xarg = va_arg(args, void *);
    } 

    /* allocate args array for proc */
    if ((proc->args = malloc(sizeof(void *) * proc->num_args)) == NULL) {
        PERROR("malloc failed for Proc Args\n");
        return errno;
    }

    /* copy over args */
    memcpy(proc->args, tmp_args, sizeof(void *) * proc->num_args);

    return 0;
}

void proc_yield(Proc *proc)
{
    Scheduler *sched;
    if (proc == NULL) sched = scheduler_self();
    else              sched = proc->sched;

    PDEBUG("yielding\n");
    scheduler_switch(&sched->curr_proc->ctx, &sched->ctx);
}

