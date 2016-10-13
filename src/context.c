
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <ucontext.h>

#include "debug.h"
#include "internal.h"
#include "context.h"

int context_create(Context **new_context, ucontext_t *ctx)
{
    Context *context;
    if ((context = malloc(sizeof(Context))) == 0) {
        PERROR("malloc failed for Context\n");
        return errno;
    }

    Scheduler *scheduler = scheduler_self();
    assert(scheduler != NULL);

    if ((context->stack = malloc(scheduler->stack_size)) == 0) {
        PERROR("malloc failed for Context stack\n");
        return errno;
    }

    context->id = 0;
    context->ctx = *ctx;
    context->func = NULL;
    context->stack_size = scheduler->stack_size;
    context->scheduler = scheduler;

    *new_context = context;

    TAILQ_INSERT_TAIL(&scheduler->readyQ, context, readyQ_next);

    return 0;
}
