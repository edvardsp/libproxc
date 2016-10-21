
#ifndef SCHEDULER_H__
#define SCHEDULER_H__

#include <stddef.h>
#include <stdint.h>
#include <ucontext.h>

#include "internal.h"

struct Scheduler {
    uint64_t     id;
    ucontext_t   ctx;
    size_t       stack_size;
    struct Proc  *curr_proc;

    /* different PROC queues */
    struct ProcQ  readyQ;
};

#endif /* SCHEDULER_H__ */

