
#ifndef INTERNAL_H__
#define INTERNAL_H__

#include <stdarg.h>
#include <ucontext.h>
#include <pthread.h>

#include "util/queue.h"
#include "util/tree.h"
#include "util/debug.h"

#define PROXC_NULL  ((void *)-1)
#define MAX_STACK_SIZE  (128 * 1024)

/* function prototype for PROC */
typedef void (*ProcFxn)(void);

/* runtime relevant structs */
struct Proc;
struct Scheduler;

/* CSP paradigm relevant structs */
struct Chan;
struct ChanEnd;

enum BuildType {
    PROC_BUILD,
    PAR_BUILD,
    SEQ_BUILD
};

struct Builder;
struct ProcBuild;
struct ParBuild;
struct SeqBuild;

struct Guard;
struct Alt;

/* typedefs for internal use */
typedef struct Proc Proc;
typedef struct Scheduler Scheduler;

typedef struct Chan Chan;
typedef struct ChanEnd ChanEnd;

typedef struct ProcBuild ProcBuild;
typedef struct ParBuild ParBuild;
typedef struct SeqBuild SeqBuild;
typedef struct Builder Builder;

typedef struct Guard Guard;
typedef struct Alt Alt;

/* queue and tree declarations */
TAILQ_HEAD(ProcQ, Proc);
RB_HEAD(ProcRB, Proc);

TAILQ_HEAD(BuilderQ, Builder);

TAILQ_HEAD(GuardQ, Guard);

/* function declarations */
int  proc_create(Proc **new_proc, ProcFxn fxn);
void proc_free(Proc *proc);
int  proc_setargs(Proc *proc, va_list args);
void proc_yield(Proc *proc);

int  scheduler_create(Scheduler **new_sched);
void scheduler_free(Scheduler *sched);
void scheduler_addproc(Proc *proc);
int  scheduler_run(void);

Chan *chan_create(void);
void chan_free(Chan *chan);
ChanEnd* chan_getend(Chan *chan);
int  chan_trywrite(ChanEnd *chan_end, void *data, size_t size);
int  chan_tryread(ChanEnd *chan_end, void *data, size_t size);
void chan_write(ChanEnd *chan_end, void *data, size_t size);
void chan_read(ChanEnd *chan_end, void *data, size_t size);
void chan_altenable(ChanEnd *chan_end, void *data, size_t size);
void chan_altdisable(ChanEnd *chan_end);

void* csp_create(enum BuildType type);
void csp_free(Builder *build);
int csp_insertchilds(size_t *num_childs, Builder *builder, struct BuilderQ *childQ, va_list vargs);
void csp_runbuild(Builder *build);

Guard* alt_guardcreate(ChanEnd *ch_end, void *out, size_t size);
void   alt_guardfree(Guard *guard);
Alt*   alt_create(void);
void   alt_free(Alt *alt);
void   alt_addguard(Alt *alt, Guard *guard);
int    alt_accept(Alt *alt, Guard *guard);
int    alt_select(Alt *alt);

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
#include "chan.h"
#include "csp.h"
#include "alt.h"

#endif /* INTERNAL_H_ */

