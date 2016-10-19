
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

static
void _scheduler_key_free(void *data)
{
    /* FIXME cleanup scheduler? */
    Scheduler *sched = (Scheduler *)data;
    scheduler_free(sched);
}

static
void _scheduler_key_create(void)
{
    ASSERT_0(pthread_key_create(&g_key_sched, _scheduler_key_free));
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

void scheduler_free(Scheduler *sched)
{
    if (sched == NULL) return;

    /* FIXME cleanup Proc's in Qs */
    memset(sched, 0, sizeof(Scheduler));
    free(sched);
}

void scheduler_addproc(Proc *proc)
{
    ASSERT_NOTNULL(proc);

    Scheduler *sched = scheduler_self();
    TAILQ_INSERT_TAIL(&sched->readyQ, proc, readyQ_next);
}

int scheduler_run(void)
{
    Scheduler *sched = scheduler_self();
    int running = 1;
    Proc *curr_proc;
    while (running) {
        PDEBUG("This is from scheduler!\n");
        //usleep(500 * 1000);

        /* find next proc to run */
        /* check ready Q */
        if (!TAILQ_EMPTY(&sched->readyQ)) {
            curr_proc = TAILQ_FIRST(&sched->readyQ);
            TAILQ_REMOVE(&sched->readyQ, curr_proc, readyQ_next);
            goto procFound;
        }

        /* no proc found, break */
        break;
procFound:
        
        curr_proc->state = PROC_RUNNING;

        sched->curr_proc = curr_proc;
        ASSERT_0(scheduler_switch(&sched->ctx, &curr_proc->ctx));
        sched->curr_proc = NULL;

        if (curr_proc->state == PROC_READY) {
            scheduler_addproc(curr_proc);
        }
        if (curr_proc->state == PROC_ENDED) {
            proc_free(curr_proc);
        }
    }

    PDEBUG("scheduler_run done\n");

    return 0;
}

