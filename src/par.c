
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

#include "debug.h"
#include "internal.h"
#include "proxc.h"

struct Par {
    uint64_t  id;
    
    struct ProcQ  joinQ;
};

typedef struct Par Par;

static
int par_create(Par **new_par)
{
    ASSERT_NOTNULL(new_par);

    Par *par;
    if ((par = malloc(sizeof(Par))) == NULL) {
        PERROR("malloc failed for Par\n");
        return errno;
    }

    TAILQ_INIT(&par->joinQ);

    *new_par = par;

    return 0;
}

static
void par_add(Par *par, FxnArg *fxn_arg)
{
    ASSERT_NOTNULL(par);
    ASSERT_NOTNULL(fxn_arg);

    /* create proc */
    /* add it */
    Proc *proc;
    proc_create(&proc, fxn_arg);
    TAILQ_INSERT_TAIL(&par->joinQ, proc, parQ_next);
}

static
void par_runjoin(Par *par)
{
    ASSERT_NOTNULL(par);
}

static
void par_free(Par *par)
{
    ASSERT_NOTNULL(par);

    Proc *proc;
    while (!TAILQ_EMPTY(&par->joinQ)) {
        proc = TAILQ_FIRST(&par->joinQ);
        TAILQ_REMOVE(&par->joinQ, proc, parQ_next);
        proc_free(proc);
    }
}

FxnArg proxc_proc(ProcFxn fxn, void *arg)
{
    return (FxnArg){ fxn, arg };
}

int proxc_par(int dummy, ...) 
{
    /* allocate PAR struct */
    Par *par;
    par_create(&par);

    /* parse args and create PAR procs */
    va_list args;
    va_start(args, dummy);
    FxnArg *fxn_arg = va_arg(args, FxnArg *);
    while (fxn_arg != NULL) {
        par_add(par, fxn_arg);
        fxn_arg = va_arg(args, FxnArg *);
    }
    va_end(args);

    /* run and join, then cleanup */
    par_runjoin(par);
    par_free(par);

    return 0;
}


