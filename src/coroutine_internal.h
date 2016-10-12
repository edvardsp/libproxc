
#ifndef COROUTINE_INTERNAL_H__
#define COROUTINE_INTERNAL_H__

#include <stdint.h>
#include <ucontext.h>
#include <pthread.h>

#include "queue.h"

#define MAX_STACK_SIZE  (128 * 1024)

struct Context;
struct Scheduler;

typedef void (*ctx_func)();

TAILQ_HEAD(ContextQ, Context);

struct Context {
    uint64_t    id;
    ucontext_t  ctx;
    ctx_func    func;
    size_t      stack_size;
    void        *stack;

    struct Scheduler  *scheduler;

    TAILQ_ENTRY(Context) readyQ_next;
};

struct Scheduler {
    uint64_t        id;
    ucontext_t      ctx;
    size_t          stack_size;
    struct Context  *current_context;

    struct ContextQ  readyQ;
};

typedef struct Context Context;
typedef struct Scheduler Scheduler;

extern pthread_key_t g_scheduler_key;
extern ucontext_t main_ctx;

int _scheduler_detect_cores(void);
int _scheduler_start(void);
void* _scheduler_mainloop(void *arg);

static inline
Scheduler* scheduler_self(void) 
{
    return pthread_getspecific(g_scheduler_key);
}

static inline
int scheduler_switch(ucontext_t *from, ucontext_t *to)
{
    return swapcontext(from, to);
}

#endif /* COROUTINE_INTERNAL_H__ */

