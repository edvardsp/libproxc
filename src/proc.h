
#ifndef PROC_H__
#define PROC_H__

#include <stddef.h>
#include <stdint.h>
#include <ucontext.h>

#include "queue.h"
#include "internal.h"

enum ProcState {
    PROC_ERROR = 0,
    PROC_READY,
    PROC_RUNNING,
    PROC_ENDED,
    PROC_PARJOIN,
    PROC_CHANWAIT
};

struct Proc {
    uint64_t        id;
    ucontext_t      ctx;
    enum ProcState  state;

    /* fxn and args */
    ProcFxn  fxn;
    size_t   num_args;
    void     **args;

    /* stack and size */
    size_t  stack_size;
    void    *stack;
    
    /* scheduler related */
    struct Scheduler   *sched;
    TAILQ_ENTRY(Proc)  readyQ_next;

    /* PAR related */
    struct Par         *par_struct;
    TAILQ_ENTRY(Proc)  parQ_next;

    /* CHAN related */
    TAILQ_ENTRY(Proc)  chanQ_next;
};

#endif /* PROC_H__ */

