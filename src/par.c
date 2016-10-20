
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "debug.h"
#include "internal.h"
#include "proxc.h"

static
int par_create(Par **new_par)
{
    ASSERT_NOTNULL(new_par);

    Par *par;
    if ((par = malloc(sizeof(Par))) == NULL) {
        PERROR("malloc failed for Par\n");
        return errno;
    }
    memset(par, 0, sizeof(Par));

    Scheduler *sched = scheduler_self();
    par->par_proc = sched->curr_proc;

    par->num_procs = 0;
    TAILQ_INIT(&par->joinQ);

    *new_par = par;

    return 0;
}

static
void par_add(Par *par, ProcFxn fxn, void *arg)
{
    ASSERT_NOTNULL(par);
    ASSERT_NOTNULL(fxn);

    /* create proc */
    /* add it */
    Proc *proc;
    proc_create(&proc, fxn, arg);
    proc->par_struct = par;
    TAILQ_INSERT_TAIL(&par->joinQ, proc, parQ_next);
    par->num_procs++;
}

static
void par_runjoin(Par *par)
{
    ASSERT_NOTNULL(par);
    
    Proc *proc;
    TAILQ_FOREACH(proc, &par->joinQ, parQ_next) {
        PDEBUG("PAR adding proc\n");
        scheduler_addproc(proc);
    }

    par->par_proc->state = PROC_PARJOIN;
    proc_yield();
    PDEBUG("par_runjoin JOINS\n");
}

static
void par_free(Par *par)
{
    ASSERT_NOTNULL(par);

    /* do NOT free procs, as the scheduler takes care of that */

    memset(par, 0, sizeof(Par));
    free(par);
}

int proxc_par(int args_start, ...) 
{
    /* allocate PAR struct */
    Par *par;
    par_create(&par);

    /* parse args and create PAR procs */
    va_list args;
    va_start(args, args_start);
    size_t num_procs = 0;
    FxnArg *fxn_arg = va_arg(args, FxnArg *);
    while (fxn_arg != NULL) {
        num_procs++;
        par_add(par, fxn_arg->fxn, fxn_arg->arg1);
        fxn_arg = va_arg(args, FxnArg *);
    }
    va_end(args);

    /* if procs, run and join PAR */
    if (num_procs > 0) {
        par_runjoin(par);
    }
    /* cleanup */
    par_free(par);

    return 0;
}


