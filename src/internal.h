
#ifndef INTERNAL_H__
#define INTERNAL_H__

#include <stdint.h>
#include <ucontext.h>
#include <pthread.h>

#include "queue.h"
#include "debug.h"

#define MAX_STACK_SIZE  (128 * 1024)

struct Proc;
struct Scheduler;

typedef void (*ProcFxn)(void);

TAILQ_HEAD(ProcQ, Proc);

enum ProcState {
    PROC_FREED = 0,
    PROC_READY,
    PROC_RUNNING,
    PROC_ENDED
};

struct Proc {
    uint64_t    id;
    ucontext_t  ctx;
    ProcFxn     fxn;
    size_t      stack_size;
    void        *stack;
    
    enum ProcState  state;

    struct Scheduler  *sched;

    TAILQ_ENTRY(Proc) readyQ_next;
};

struct Scheduler {
    uint64_t     id;
    ucontext_t   ctx;
    size_t       stack_size;
    struct Proc  *curr_proc;

    struct ProcQ  readyQ;
};

typedef struct Proc Proc;
typedef struct Scheduler Scheduler;

extern pthread_key_t g_key_sched;

int proc_create(Proc **new_proc, ProcFxn fxn);
void proc_free(Proc *proc);

int scheduler_create(Scheduler **new_sched);
void scheduler_free(Scheduler *sched);
void scheduler_addproc(Proc *proc);
int scheduler_run(void);
void scheduler_yield(void);

static inline
Scheduler* scheduler_self(void)
{
    Scheduler *sched = pthread_getspecific(g_key_sched);
    ASSERT_NOTNULL(sched);
    return sched;
}

static inline
int scheduler_switch(ucontext_t *from, ucontext_t *to)
{
    return swapcontext(from, to);
}

#endif /* INTERNAL_H_ */

