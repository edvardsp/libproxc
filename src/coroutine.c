
#include <assert.h>
#include <ucontext.h>

#include "coroutine_internal.h"

void scheduler_start(void)
{
    static volatile int in_ctx = 0;
    assert(getcontext(&main_ctx) == 0);
    if (in_ctx) goto main_fxn;
    in_ctx = 1;

    _scheduler_detect_cores();
    _scheduler_start();

    // start scheduler for this pthread, no return
    ucontext_t ctx;
    _scheduler_mainloop((void *)0);

main_fxn:
    return;
}

void scheduler_end(void)
{
    return;
}

void scheduler_yield(void)
{
    Scheduler *scheduler = scheduler_self();
    scheduler_switch(&scheduler->current_context->ctx, &scheduler->ctx);
}

