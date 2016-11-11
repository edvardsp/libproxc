
#ifndef SCHEDULER_H__
#define SCHEDULER_H__

#include <stddef.h>
#include <stdint.h>
#include <ucontext.h>

#include "internal.h"

struct Scheduler {
    uint64_t     id;
    Ctx          ctx;
    size_t       stack_size;
    struct Proc  *curr_proc;

    /* different PROC queues and trees */
    struct ProcQ   readyQ;
    struct ProcQ   altQ;
    struct ProcRB  sleepRB;
};

#endif /* SCHEDULER_H__ */

