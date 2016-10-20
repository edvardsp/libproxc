
#ifndef INTERNAL_H__
#define INTERNAL_H__

#include <stdint.h>
#include <stdarg.h>
#include <ucontext.h>
#include <pthread.h>

#include "queue.h"
#include "debug.h"

#define MAX_STACK_SIZE  (128 * 1024)

/* function prototype for PROC */
typedef void (*ProcFxn)(void);

/* runtime relevant structs */
struct Proc;
struct Scheduler;

/* CSP paradigm relevant structs */
struct Par;
struct Chan; /* FIXME */

/* typedefs for internal use */
typedef struct Proc Proc;
typedef struct Scheduler Scheduler;

typedef struct Par Par;
typedef struct Chan Chan;

/* queue declarations */
TAILQ_HEAD(ProcQ, Proc);

/* function declarations */
int  proc_create(Proc **new_proc, ProcFxn fxn);
void proc_free(Proc *proc);
int  proc_setargs(Proc *proc, va_list args);
void proc_yield(void);

int  scheduler_create(Scheduler **new_sched);
void scheduler_free(Scheduler *sched);
void scheduler_addproc(Proc *proc);
int  scheduler_run(void);

int  par_create(Par **new_par);
void par_free(Par *par);
void par_runjoin(Par *par);

/* extern and static inline functions */
extern pthread_key_t g_key_sched;

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
    int ret = swapcontext(from, to);
    ASSERT_0(ret);
    return ret;
}

/* implementation of corresponding types and structs */
/* must be after the declaration of the types */
#include "proc.h"
#include "scheduler.h"
#include "par.h"

#endif /* INTERNAL_H_ */

