
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
    ASSERT_IS(proc->state, PROC_READY);

    PDEBUG("scheduler_addproc called\n");
       
    Scheduler *sched = proc->sched;
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

        /* from here, a PROC is found to resume */
procFound:
        ASSERT_NOTNULL(curr_proc);
        
        sched->curr_proc = curr_proc;
        sched->curr_proc->state = PROC_RUNNING;

        scheduler_switch(&sched->ctx, &sched->curr_proc->ctx);

        switch (sched->curr_proc->state) {
        case PROC_RUNNING:
        case PROC_READY:
            sched->curr_proc->state = PROC_READY;
            scheduler_addproc(sched->curr_proc);
            break;
        case PROC_ENDED: {
            /*  check if curr_proc is in a PAR */
            Par *par = sched->curr_proc->par_struct;
            if (par != NULL) {
                /* if last one, resume PAR joining PROC */
                /* FIXME atomic */
                if (--par->num_procs == 0) {
                    par->par_proc->state = PROC_READY;
                    scheduler_addproc(par->par_proc);
                }
            }

            /* cleanup */
            proc_free(sched->curr_proc);
        }
            break;
        case PROC_PARJOIN:
            /* do nothing, as this proc will be revived  */
            break;
        case PROC_CHANWAIT:
            /* do nothing, the other end of CHAN will re-add it */
            break;
        default:
            break;
        }

        sched->curr_proc = NULL;
    }

    PDEBUG("scheduler_run done\n");

    return 0;
}

