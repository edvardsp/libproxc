
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

typedef void (*ProcFxn)(void *);

struct FxnArg {
    ProcFxn  fxn;
    void     *arg;
};

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
    void        *arg;
    size_t      stack_size;
    void        *stack;

    enum ProcState  state;
    
    /* scheduler related */
    struct Scheduler  *sched;
    TAILQ_ENTRY(Proc) readyQ_next;

    /* CSP structure related */
    TAILQ_ENTRY(Proc) parQ_next;
};

struct Scheduler {
    uint64_t     id;
    ucontext_t   ctx;
    size_t       stack_size;
    struct Proc  *curr_proc;

    struct ProcQ  readyQ;
};

typedef struct FxnArg FxnArg;
typedef struct Proc Proc;
typedef struct Scheduler Scheduler;

extern pthread_key_t g_key_sched;

int proc_create(Proc **new_proc, FxnArg *fxn_arg);
void proc_free(Proc *proc);
void proc_yield(void);

int scheduler_create(Scheduler **new_sched);
void scheduler_free(Scheduler *sched);
void scheduler_addproc(Proc *proc);
int scheduler_run(void);

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

