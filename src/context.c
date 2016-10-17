
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ucontext.h>

#include "debug.h"
#include "internal.h"

/* static */
/* void _context_threadfxn(Context *ctx) */
/* { */
/*     ASSERT_NOTNULL(ctx); */
/*     /1* do scheduler stuff *1/ */
/*     /1* call ctx->fxn() *1/ */
/*     /1* when fxn returns, do *1/ */
/*         /1* maybe clean up context? *1/ */
/*         /1* return control to scheduler *1/ */
/* } */

int context_create(Context **new_ctx, CtxFxn fxn)
{
    ASSERT_NOTNULL(new_ctx);

    Context *ctx;
    if ((ctx = malloc(sizeof(Context))) == NULL) {
        PERROR("malloc failed for Context\n");
        return errno;
    }
    memset(ctx, 0, sizeof(Context));

    Scheduler *sched = scheduler_self();
    if ((ctx->stack = malloc(sched->stack_size)) == NULL) {
        free(ctx);
        PERROR("malloc failed for Context stack\n");
        return errno;
    }

    /* if (posix_memalign(&ctx->stack, (size_t)getpagesize(), sched->stack_size)) { */
    /*     PERROR("posix_memalign failed\n"); */
    /*     return errno; */
    /* } */

    ctx->fxn = fxn;
    ctx->stack_size = sched->stack_size;
    ctx->sched = sched;

    /* configure context */
    ASSERT_0(getcontext(&ctx->ctx));
    ctx->ctx.uc_link          = &sched->ctx;
    ctx->ctx.uc_stack.ss_sp   = ctx->stack;
    ctx->ctx.uc_stack.ss_size = ctx->stack_size;
    makecontext(&ctx->ctx, fxn, 0);

    *new_ctx = ctx;

    return 0;
}
