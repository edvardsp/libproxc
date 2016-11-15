
#ifndef INTERNAL_H__
#define INTERNAL_H__

#include <stdarg.h>
#include <pthread.h>

#include "util/queue.h"
#include "util/tree.h"
#include "util/debug.h"

#include "context.h"


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
// Ctx is defined in context.h, as it is architecture dependent
typedef struct Proc Proc;
typedef struct Scheduler Scheduler;

typedef struct ChanEnd ChanEnd;
typedef struct Chan Chan;

typedef struct ProcBuild ProcBuild;
typedef struct ParBuild ParBuild;
typedef struct SeqBuild SeqBuild;
typedef struct Builder Builder;

typedef struct Guard Guard;
typedef struct Alt Alt;

/* queue and tree declarations */
TAILQ_HEAD(ProcQ, Proc);
RB_HEAD(ProcRB, Proc);

TAILQ_HEAD(ChanEndQ, ChanEnd);

TAILQ_HEAD(BuilderQ, Builder);

TAILQ_HEAD(GuardQ, Guard);

/* function declarations */
void ctx_init(Ctx *ctx, Proc *proc);
void ctx_switch(Ctx *from, Ctx *to);
void ctx_madvise(Proc *proc);

void  proc_mainfxn(Proc *proc);
Proc* proc_self(void);
int   proc_create(Proc **new_proc, ProcFxn fxn);
void  proc_free(Proc *proc);
int   proc_setargs(Proc *proc, va_list args);
void  proc_yield(Proc *proc);

Scheduler* scheduler_self(void);
int  scheduler_create(Scheduler **new_sched);
void scheduler_free(Scheduler *sched);
void scheduler_addproc(Proc *proc);
void scheduler_addsleep(Proc *proc);
void scheduler_remsleep(Proc *proc);
int  scheduler_run(void);

Chan *chan_create(size_t size);
void chan_free(Chan *chan);
int  chan_write(Chan *chan, void *data, size_t size);
int  chan_read(Chan *chan, void *data, size_t size);
int  chan_altread(Chan *chan, Guard *guard, void *data, size_t size);

void* csp_create(enum BuildType type);
void csp_free(Builder *build);
int csp_insertchilds(size_t *num_childs, Builder *builder, struct BuilderQ *childQ, va_list vargs);
void csp_runbuild(Builder *build);
void csp_cleanupbuild(Builder *build);
void csp_parsebuild(Builder *build);

Guard* alt_guardcreate(Chan *ch, void *data, size_t size);
void   alt_guardfree(Guard *guard);
Alt*   alt_create(void);
void   alt_free(Alt *alt);
void   alt_addguard(Alt *alt, Guard *guard);
int    alt_accept(Guard *guard);
int    alt_enable(Guard *guard);
void   alt_disable(Guard *guard);
int    alt_select(Alt *alt);

/* implementation of corresponding types and structs */
/* must be after the declaration of the types */
#include "proc.h"
#include "scheduler.h"
#include "chan.h"
#include "csp.h"
#include "alt.h"

#endif /* INTERNAL_H_ */

