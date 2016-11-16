
#ifndef SCHEDULER_H__
#define SCHEDULER_H__

#include <stddef.h>
#include <stdint.h>
#include <ucontext.h>

#include "internal.h"

struct Scheduler {
    uint64_t  id;
    Ctx       ctx;

    size_t  stack_size;
    size_t  page_size; 

    struct Proc  *curr_proc;

    /* different PROC queues and trees */
    struct ProcQ   readyQ;
    struct ProcQ   altQ;

    struct {
        size_t  num;
        struct ProcRB_sleep  RB;
    } sleep;
    struct {
        size_t num;
        struct GuardRB_altsleep  RB;
    } altsleep;
};

#endif /* SCHEDULER_H__ */

