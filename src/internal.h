
#ifndef INTERNAL_H__
#define INTERNAL_H__

#include <stdint.h>
#include <ucontext.h>
#include <pthread.h>

#include "queue.h"

#define MAX_STACK_SIZE  (128 * 1024)

struct Context;
struct Scheduler;

typedef void (*CtxFxn)(void);

TAILQ_HEAD(ContextQ, Context);

struct Context {
    uint64_t    id;
    ucontext_t  ctx;
    CtxFxn      fxn;
    size_t      stack_size;
    void        *stack;

    struct Scheduler  *sched;

    TAILQ_ENTRY(Context) readyQ_next;
};

struct Scheduler {
    uint64_t        id;
    ucontext_t      ctx;
    size_t          stack_size;
    struct Context  *curr_ctx;

    struct ContextQ  readyQ;
};

typedef struct Context Context;
typedef struct Scheduler Scheduler;

extern pthread_key_t g_key_sched;

int context_create(Context **new_ctx, CtxFxn fxn);

int scheduler_create(Scheduler **new_sched);
int scheduler_addctx(Context *ctx);
int scheduler_run(void);

static inline
Scheduler* scheduler_self(void)
{
    return pthread_getspecific(g_key_sched);
}

static inline
int scheduler_switch(ucontext_t *from, ucontext_t *to)
{
    return swapcontext(from, to);
}

#endif /* INTERNAL_H__ */

