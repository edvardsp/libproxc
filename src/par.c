
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "debug.h"
#include "internal.h"

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

void par_free(Par *par)
{
    ASSERT_NOTNULL(par);

    /* do NOT free procs, as the scheduler takes care of that */

    memset(par, 0, sizeof(Par));
    free(par);
}

void par_runjoin(Par *par)
{
    ASSERT_NOTNULL(par);
    
    Proc *proc;
    TAILQ_FOREACH(proc, &par->joinQ, parQ_next) {
        PDEBUG("PAR adding proc\n");
        scheduler_addproc(proc);
    }

    par->par_proc->state = PROC_PARJOIN;
    proc_yield(par->par_proc);
    PDEBUG("par_runjoin JOINS\n");
}


