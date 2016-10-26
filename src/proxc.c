
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <pthread.h>
#include <features.h>

#include "debug.h"
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

/*
 * Variadic args is a zero terminated list
 * of void * arguments to fxn. In fxn context,
 * args are accessed through proxc_argn() method.
 */
void* proxc_proc(ProcFxn fxn, ...)
{
    Proc *proc;
    ASSERT_0(proc_create(&proc, fxn));

    va_list args;
    va_start(args, fxn);
    ASSERT_0(proc_setargs(proc, args));
    va_end(args);

    return proc;
} 

/*
 * Variadic args are zero terminated list
 * of pointers to allready allocated PROCS.
 * args_start is only for va_start() call.
 */
int proxc_par(int args_start, ...) 
{
    /* allocate PAR struct */
    Par *par;
    ASSERT_0(par_create(&par));

    /* parse args and add PROCS to joinQ */
    va_list args;
    va_start(args, args_start);
    Proc *proc = va_arg(args, Proc *);
    while (proc != NULL) {
        par->num_procs++;
        proc->par_struct = par;
        TAILQ_INSERT_TAIL(&par->joinQ, proc, parQ_next);

        proc = va_arg(args, Proc *);
    }
    va_end(args);

    /* if procs, run and join PAR */
    if (par->num_procs > 0) {
        PDEBUG("PAR starting with %zu PROCS\n", par->num_procs);
        par_runjoin(par);
    }
    /* cleanup */
    par_free(par);

    return 0;
}

void* proxc_argn(size_t n)
{
    Proc *proc = scheduler_self()->curr_proc;
    /* if n is index out of range, return NULL */
    if (n >= proc->num_args) return NULL;
    
    return proc->args[n];
}

int proxc_ch_open(int arg_start, ...)
{
    va_list args;
    va_start(args, arg_start);
    Chan **new_chan = (Chan **)va_arg(args, Chan **);
    while (new_chan != NULL) {
        chan_create(new_chan);
        new_chan = (Chan **)va_arg(args, Chan **);
    }
    va_end(args);

    return 0;
}

int proxc_ch_close(int arg_start, ...)
{
    va_list args;
    va_start(args, arg_start);
    Chan *chan = (Chan *)va_arg(args, Chan *);
    while (chan != NULL) {
        chan_free(chan);
        chan = (Chan *)va_arg(args, Chan *);
    }
    va_end(args);

    return 0;
}

