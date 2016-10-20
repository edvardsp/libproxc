
#ifndef SCHEDULER_H__
#define SCHEDULER_H__

#include "internal.h"

TAILQ_HEAD(ProcQ, Proc);

struct Scheduler {
    uint64_t     id;
    ucontext_t   ctx;
    size_t       stack_size;
    struct Proc  *curr_proc;

    struct ProcQ  readyQ;
};

#endif /* SCHEDULER_H__ */

