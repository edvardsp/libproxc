
#include <assert.h>
#include <ucontext.h>

#include "internal.h"
#include "debug.h"

void proxc_start(void)
{
    static volatile int in_ctx = 0;
    ASSERT_0(
        getcontext(&main_ctx)
    );
    if (in_ctx) goto main_fxn;
    in_ctx = 1;

    _scheduler_detect_cores();
    _scheduler_start();

    // start scheduler for this pthread, no return
    
    _scheduler_mainloop((void *)0);

main_fxn:
    return;
}

void proxc_end(void)
{
    return;
}

void scheduler_yield(void)
{
    Scheduler *scheduler = scheduler_self();
    scheduler_switch(&scheduler->current_context->ctx, &scheduler->ctx);
}

