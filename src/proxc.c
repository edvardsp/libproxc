
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sched.h>
#include <pthread.h>
#include <features.h>

#include "util/debug.h"
#include "internal.h"

static 
int _proxc_setaffinity(unsigned long core_id)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

static
void* _proxc_pthreadfxn(void *arg)
{
    unsigned long core_id = (unsigned long)arg;

    // Set pthread affinity
    ASSERT_0(_proxc_setaffinity(core_id));

    Scheduler *sched;
    scheduler_create(&sched);
    /* FIXME put scheduler on wait */

    return NULL;
}

void proxc_start(ProcFxn fxn)
{
    /* find number of online cores on architecture */
    long nprocessors_onln = sysconf(_SC_NPROCESSORS_ONLN);
    if (nprocessors_onln < 0) {
        PERROR("sysconf failed, default to 1\n");
        nprocessors_onln = 1;
    }
    PDEBUG("Found %ld online cores\n", nprocessors_onln);

    /* spawn pthreads for the other avaiable cores */
    pthread_t *pthreads = NULL;
    if (nprocessors_onln > 1) {
        unsigned long num_pthreads = (unsigned long)(nprocessors_onln - 1);
        if ((pthreads = malloc(sizeof(pthread_t) * num_pthreads)) == 0) {
            PERROR("malloc failed for pthread_t\n");
            exit(EXIT_FAILURE);
        }
        for (unsigned long i = 0; i < num_pthreads; i++) {
            /* core id, is +1 because id=0 is this pthread */
            void *arg = (void *)(i + 1);
            ASSERT_0(pthread_create(&pthreads[i], NULL, _proxc_pthreadfxn, arg));
        }
    }
    
    /* call pthreadfxn as if this pthread just spawned */
    ASSERT_0(_proxc_setaffinity(0));

    /* create scheduler for this pthread */
    Scheduler *sched;
    scheduler_create(&sched);
    Proc *proc;
    proc_create(&proc, fxn);
    scheduler_addproc(proc);
    scheduler_run();

    /* when returned from _proxc_pthreadfxn, assume program to be finished */

    if (pthreads != NULL) {
        /* FIXME cleanup other pthreads */
    }

    /* FIXME cleanup this pthread */

}

void* proxc_argn(size_t n)
{
    Proc *proc = scheduler_self()->curr_proc;
    /* if n is index out of range, return NULL */
    if (n >= proc->args.num) return NULL;
    
    return proc->args.ptr[n];
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
    if ((builder = csp_create(PROC_BUILD)) == NULL) {
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

    return (Builder *)builder;
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
    if ((builder = csp_create(PAR_BUILD)) == NULL) {
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
    ret = csp_insertchilds(&builder->num_childs, (Builder *)builder, 
                           &builder->childQ, args);
    PDEBUG("PAR build, %zu childs\n", builder->num_childs);
    ASSERT_0(ret);
    va_end(args);

    /* FIXME cleanup on NULL */

    return (Builder *)builder;
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
    if ((builder = csp_create(SEQ_BUILD)) == NULL) {
        PERROR("malloc failed for SeqBuild\n");
        return NULL;
    }

    /* set builder members */
    builder->num_childs = 0;
    TAILQ_INIT(&builder->childQ);

    va_list args;
    va_start(args, arg_start);
    int ret;
    ret = csp_insertchilds(&builder->num_childs, (Builder *)builder, 
                           &builder->childQ, args);
    PDEBUG("SEQ build, %zu childs\n", builder->num_childs);
    ASSERT_0(ret);
    builder->curr_build = TAILQ_FIRST(&builder->childQ);
    va_end(args);

    /* FIXME cleanup on error */

    return (Builder *)builder;
}


int proxc_go(Builder *root)
{
    ASSERT_NOTNULL(root);

    Builder *build = (Builder *)root;

    /* this notifies scheduler when the bottom of the build is reached */
    build->header.is_root = 1;
    build->header.run_proc = NULL;

    csp_runbuild(build);

    return 0;
}

int proxc_run(Builder *root)
{
    ASSERT_NOTNULL(root);

    Builder *build = (Builder *)root; 
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

Guard* proxc_guard(int cond, ChanEnd *ch_end, void *out, size_t size)
{
    ASSERT_NOTNULL(ch_end);
    
    /* if cond is true, return a guard, else NULL */
    return (cond) 
        ? alt_guardcreate(ch_end, out, size)
        : NULL;
}

int proxc_alt(int arg_start, ...)
{
    Alt *alt = alt_create();
    if (alt == NULL) {
        return -1;
    }

    va_list args;
    va_start(args, arg_start);
    Guard *guard = va_arg(args, Guard *);
    while (guard != PROXC_NULL) {
        alt_addguard(alt, guard);
        guard = va_arg(args, Guard *);
    }
    va_end(args);

    /* wait on guards */
    int key = alt_select(alt);

    /* cleanup */
    alt_free(alt);

    return key;
}

int proxc_ch_open(int arg_start, ...)
{
    va_list args;
    va_start(args, arg_start);
    Chan **new_chan = (Chan **)va_arg(args, Chan **);
    while (new_chan != PROXC_NULL) {
        if (new_chan == NULL) {
            errno = EPERM;
            PERROR("new_chan in proxc_ch_open is NULL\n");
            break;
        }
        *new_chan = chan_create();
        PDEBUG("new CHAN is opened\n");
        new_chan = (Chan **)va_arg(args, Chan **);
    } 
    va_end(args);

    /* FIXME on error */

    return 0;
}

int proxc_ch_close(int arg_start, ...)
{
    va_list args;
    va_start(args, arg_start);
    Chan *chan = (Chan *)va_arg(args, Chan *);
    while (chan != PROXC_NULL) {
        chan_free(chan);
        PDEBUG("CHAN is closed\n");
        chan = (Chan *)va_arg(args, Chan *);
    } 
    va_end(args);

    return 0;
}

