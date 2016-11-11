
#ifndef PROC_H__
#define PROC_H__

#include <stddef.h>
#include <stdint.h>

#include "util/queue.h"
#include "internal.h"

enum ProcState {
    PROC_ERROR = 0,
    PROC_READY,
    PROC_RUNNING,
    PROC_ENDED,
    PROC_CHANWAIT,
    PROC_RUNWAIT,
    PROC_ALTWAIT
};

struct Proc {
    uint64_t        id;
    Ctx             ctx;
    enum ProcState  state;

    /* fxn and args */
    ProcFxn  fxn;
    struct {
        size_t  num;
        void    **ptr;
    } args;

    /* stack and size */
    struct {
        size_t  size;
        void    *ptr;
    } stack;
    
    /* scheduler related */
    struct Scheduler   *sched;
    TAILQ_ENTRY(Proc)  readyQ_next;
    TAILQ_ENTRY(Proc)  altQ_next;
    RB_ENTRY(Proc)     sleepRB_node;

    /* Par/Seq/Proc-builder related */
    struct ProcBuild  *proc_build;
};

#endif /* PROC_H__ */

