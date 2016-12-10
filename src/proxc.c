
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>

#include "internal.h"

void proxc_start(ProcFxn fxn)
{
    /* create scheduler for this pthread */
    Scheduler *sched;
    scheduler_create(&sched);
    Proc *proc;
    proc_create(&proc, fxn);
    proc->sched->main_proc = proc;
    scheduler_addready(proc);

    scheduler_run();

    scheduler_free(sched);
}

void proxc_exit(void)
{
    Scheduler *sched = scheduler_self();
    sched->is_exit = 1;
    proc_yield(sched->curr_proc);
}

void* proxc_argn(size_t n)
{
    Proc *proc = proc_self();
    /* if n is index out of range, return NULL */
    return (n < proc->args.num) 
        ? proc->args.ptr[n]
        : NULL;
}

void proxc_yield(void)
{
    proc_yield(NULL);
}

void proxc_sleep(uint64_t usec)
{
    Proc *proc = proc_self();
    if (usec > 0) {
        proc->sleep_us = gettimestamp() + usec;
        scheduler_addsleep(proc);
    }
    proc_yield(proc);
    proc->sleep_us = 0;
}

/*
 * Variadic args is a PROXC_NULL terminated list
 * of void * arguments to fxn. In fxn context,
 * args are accessed through proxc_argn() method.
 */
Builder* proxc_proc(ProcFxn fxn, ...)
{
    ASSERT_NOTNULL(fxn);

    PDEBUG("PROC build\n");

    /* alloc proc struct */
    int ret;
    Proc *proc;
    ret = proc_create(&proc, fxn);
    ASSERT_0(ret);

    /* alloc builder struct */
    ProcBuild *builder;
    if (!(builder = csp_create(PROC_BUILD))) {
        PERROR("malloc failed for ProcBuild\n");
        proc_free(proc);
        return NULL;
    }

    /* set args list for fxn */
    va_list args;
    va_start(args, fxn);
    ret = proc_setargs(proc, args);
    ASSERT_0(ret);
    va_end(args);

    /* set builder members */
    builder->proc = proc;

    /* and ready scheduler for build */
    proc->proc_build = builder;

    return BUILDER_CAST(builder, Builder*);
} 


/*
 * Variadic args is a PROXC_NULL terminated list
 * of pointers to allready allocated PROCS.
 * args_start is only for va_start() call.
 */
Builder* proxc_par(int args_start, ...) 
{

    /* alloc builder struct */
    ParBuild *builder;
    if (!(builder = csp_create(PAR_BUILD))) {
        PERROR("malloc failed for ParBuild\n");
        return NULL;
    }
    
    /* set builder members */
    builder->num_childs = 0;
    TAILQ_INIT(&builder->childQ);

    /* parse args and insert them into builderQ */
    va_list args;
    va_start(args, args_start);
    int ret;
    ret = csp_insertchilds(&builder->num_childs, BUILDER_CAST(builder, Builder*), 
                           &builder->childQ, args);
    PDEBUG("PAR build, %zu childs\n", builder->num_childs);
    ASSERT_0(ret);
    va_end(args);

    /* FIXME cleanup on NULL */

    return BUILDER_CAST(builder, Builder*);
}

/*
 * Variadic args is a PROXC_NULL terminated list
 * of pointers to allready allocated PROCS.
 * args_start is only for va_start() call.
 */
Builder* proxc_seq(int arg_start, ...)
{

    /* alloc builder struct */
    SeqBuild *builder;
    if (!(builder = csp_create(SEQ_BUILD))) {
        PERROR("malloc failed for SeqBuild\n");
        return NULL;
    }

    /* set builder members */
    builder->num_childs = 0;
    TAILQ_INIT(&builder->childQ);

    va_list args;
    va_start(args, arg_start);
    int ret;
    ret = csp_insertchilds(&builder->num_childs, BUILDER_CAST(builder, Builder*), 
                           &builder->childQ, args);
    PDEBUG("SEQ build, %zu childs\n", builder->num_childs);
    ASSERT_0(ret);
    builder->curr_build = TAILQ_FIRST(&builder->childQ);
    va_end(args);

    /* FIXME cleanup on error */

    return BUILDER_CAST(builder, Builder*);
}


int proxc_go(Builder *root)
{
    ASSERT_NOTNULL(root);

    Builder *build = BUILDER_CAST(root, Builder*);

    /* this notifies scheduler when the bottom of the build is reached */
    build->header.is_root = 1;
    build->header.run_proc = NULL;

    csp_runbuild(build);

    return 0;
}

int proxc_run(Builder *root)
{
    ASSERT_NOTNULL(root);

    Builder *build = BUILDER_CAST(root, Builder*); 
    Scheduler *sched = scheduler_self();

    /* this triggers rescheduling of this PROC when RUN tree is done */
    build->header.is_root = 1;
    build->header.run_proc = sched->curr_proc;

    PDEBUG("RUN building CSP tree\n");
    csp_runbuild(build);
    PDEBUG("RUN built CSP tree\n");

    sched->curr_proc->state = PROC_RUNWAIT;
    proc_yield(sched->curr_proc);

    PDEBUG("RUN CSP tree finished\n");

    /* cleanup is done by the scheduler */

    return 0;
}

Guard* proxc_guardchan(int cond, Chan *chan, void *out, size_t size)
{
    /* if cond is true, return ChanGuard */
    return (cond) 
        ? alt_guardcreate(GUARD_CHAN, 0, chan, out, size)
        /* else NULL */
        : NULL;
}

Guard* proxc_guardtime(int cond, uint64_t usec)
{
    /* if cond is true and usec > 0, return TimerGuard */
    return (cond && usec > 0)
        ? alt_guardcreate(GUARD_TIME, usec + gettimestamp(), NULL, NULL, 0)
        /* if cond is true and usec == 0, return SkipGuard */
        : (cond)
            ? alt_guardcreate(GUARD_SKIP, 0, NULL, NULL, 0)
            /* else NULL */
            : NULL;
}

Guard* proxc_guardskip(int cond)
{
    /* if cond is true, return SkipGuard*/
    return (cond)
        ? alt_guardcreate(GUARD_SKIP, 0, NULL, NULL, 0)
        /* else NULL */
        : NULL;
}

int proxc_alt(int arg_start, ...)
{
    Alt alt;
    alt_init(&alt);

    va_list args;
    va_start(args, arg_start);
    Guard *guard = va_arg(args, Guard *);
    while (guard != PROXC_NULL) {
        alt_addguard(&alt, guard);
        guard = va_arg(args, Guard *);
    }
    va_end(args);

    /* wait on guards */
    int key = alt_select(&alt);

    /* cleanup */
    alt_cleanup(&alt);

    return key;
}

Chan* proxc_chopen(size_t size)
{
    return chan_create(size); 
}

void proxc_chclose(Chan *chan)
{
    chan_free(chan);
}

int proxc_chwrite(Chan *chan, void *data, size_t size)
{
    return chan_write(chan, data, size);
}

int proxc_chread(Chan *chan, void *data, size_t size)
{
    return chan_read(chan, data, size);
}

