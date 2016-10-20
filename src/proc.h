
#ifndef PROC_H__
#define PROC_H__

#include "internal.h"

enum ProcState {
    PROC_FREED = 0,
    PROC_READY,
    PROC_RUNNING,
    PROC_ENDED,
    PROC_PARJOIN
};

struct Proc {
    uint64_t    id;
    ucontext_t  ctx;
    ProcFxn     fxn;
    size_t      num_args;
    void        **args;
    size_t      stack_size;
    void        *stack;

    enum ProcState  state;
    
    /* scheduler related */
    struct Scheduler  *sched;
    TAILQ_ENTRY(Proc) readyQ_next;

    /* CSP structure related */
    struct Par        *par_struct;
    TAILQ_ENTRY(Proc) parQ_next;
};

#endif /* PROC_H__ */

