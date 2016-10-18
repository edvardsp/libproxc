
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sched.h>
#include <unistd.h>
#include <ucontext.h>
#include <pthread.h>

#include "debug.h"
#include "queue.h"
#include "internal.h"

// Holds corresponding scheduler for each pthread
pthread_key_t g_key_sched;
static pthread_once_t g_key_once = PTHREAD_ONCE_INIT;

/* static pthread_t *g_kernel_threads = NULL; */
/* static Scheduler *g_scheds = NULL; */

/* static unsigned long g_num_cores = 0; */

static
void _scheduler_key_create(void)
{
    ASSERT_0(pthread_key_create(&g_key_sched, NULL));
}

int scheduler_create(Scheduler **new_sched)
{
    ASSERT_NOTNULL(new_sched);

    Scheduler *sched;
    if ((sched = malloc(sizeof(Scheduler))) == NULL) {
        PERROR("malloc failed for Scheduler\n");
        return errno;
    }
    memset(sched, 0, sizeof(Scheduler));

    sched->stack_size = MAX_STACK_SIZE;

    /* FIXME */
    // Save scheduler for this pthread
    ASSERT_0(pthread_once(&g_key_once, _scheduler_key_create));
    ASSERT_0(pthread_setspecific(g_key_sched, sched));
    // and context
    ASSERT_0(getcontext(&sched->ctx));

    TAILQ_INIT(&sched->readyQ);

    *new_sched = sched;

    return 0;
}

int scheduler_addproc(Proc *proc)
{
    ASSERT_NOTNULL(proc);

    Scheduler *sched = scheduler_self();
    TAILQ_INSERT_TAIL(&sched->readyQ, proc, readyQ_next);
    return 0;
}

int scheduler_run(void)
{
    Scheduler *sched = scheduler_self();

    int running = 1;
    Proc *curr_proc;
    while (running) {
        curr_proc = TAILQ_LAST(&sched->readyQ, ProcQ);
        if (curr_proc == NULL) break;

        PDEBUG("This is from scheduler!\n");

        sched->curr_proc = curr_proc;
        ASSERT_0(scheduler_switch(&sched->ctx, &curr_proc->ctx));
        sched->curr_proc = NULL;
    }

    return 0;
}

 void scheduler_yield(void)
{
    Scheduler *sched = scheduler_self();

    PDEBUG("yielding\n");
    ASSERT_0(scheduler_switch(&sched->curr_proc->ctx, &sched->ctx));
}

