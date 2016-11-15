
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sched.h>
#include <unistd.h>
#include <ucontext.h>
#include <pthread.h>

#include "internal.h"

// Holds corresponding scheduler for each pthread
static pthread_key_t g_key_sched;
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
    int ret;
    ret = pthread_key_create(&g_key_sched, _scheduler_key_free);
    ASSERT_0(ret);
}

Scheduler* scheduler_self(void)
{
    Scheduler *sched = pthread_getspecific(g_key_sched);
    ASSERT_NOTNULL(sched);
    return sched;
}

static
int _scheduler_sleep_cmp(Proc *p1, Proc *p2)
{
    ASSERT_NOTNULL(p1);
    ASSERT_NOTNULL(p2);
    return (p1->sleep_us  < p2->sleep_us) ? -1
         : (p1->sleep_us == p2->sleep_us) ?  0
                                          :  1;
}

RB_GENERATE(ProcRB_sleep, Proc, sleepRB_node, _scheduler_sleep_cmp)
RB_GENERATE(ProcRB_altsleep, Proc, sleepRB_node, _scheduler_sleep_cmp)

int scheduler_create(Scheduler **new_sched)
{
    ASSERT_NOTNULL(new_sched);

    Scheduler *sched;
    if (!(sched = malloc(sizeof(Scheduler)))) {
        PERROR("malloc failed for Scheduler\n");
        return errno;
    }

    /* configure members */
    sched->stack_size = MAX_STACK_SIZE;
    sched->page_size  = (size_t)sysconf(_SC_PAGESIZE);

    // Save scheduler for this pthread
    int ret;
    ret = pthread_once(&g_key_once, _scheduler_key_create);
    ASSERT_0(ret);
    ret = pthread_setspecific(g_key_sched, sched);
    ASSERT_0(ret);

    // and context
    ctx_init(&sched->ctx, NULL);

    TAILQ_INIT(&sched->readyQ);
    TAILQ_INIT(&sched->altQ);
    sched->sleep.num = 0;
    RB_INIT(&sched->sleep.RB);
    sched->altsleep.num = 0;
    RB_INIT(&sched->altsleep.RB);

    *new_sched = sched;

    return 0;
}

void scheduler_free(Scheduler *sched)
{
    if (!sched) return;

    /* FIXME cleanup Proc's in Qs */
    free(sched);
}

void scheduler_addready(Proc *proc)
{
    ASSERT_NOTNULL(proc);

    PDEBUG("scheduler_addready called\n");
       
    Scheduler *sched = proc->sched;
    proc->state = PROC_READY;
    TAILQ_INSERT_TAIL(&sched->readyQ, proc, readyQ_next);
}

void scheduler_remready(Proc *proc)
{
    ASSERT_NOTNULL(proc);

    Scheduler *sched = proc->sched;
    TAILQ_REMOVE(&sched->readyQ, proc, readyQ_next);
}

void scheduler_addsleep(Proc *proc)
{
    ASSERT_NOTNULL(proc);

    PDEBUG("scheduler_addsleep called\n");

    Scheduler *sched = proc->sched;
    proc->state = PROC_SLEEPING;
    size_t num_tries = 0;
    enum { MAX_TRIES = 1000 };
    while (RB_INSERT(ProcRB_sleep, &sched->sleep.RB, proc) && (num_tries++ < MAX_TRIES)) {
        /* this means there is a key collision, increment usec */
        ++proc->sleep_us;
    }
    ++sched->sleep.num;
    ASSERT_TRUE(num_tries < MAX_TRIES);
}

void scheduler_remsleep(Proc *proc)
{
    ASSERT_NOTNULL(proc);

    Scheduler *sched = proc->sched;
    RB_REMOVE(ProcRB_sleep, &sched->sleep.RB, proc);
    --sched->sleep.num;
}

static
void _scheduler_wakeup(Scheduler *sched)
{
    ASSERT_NOTNULL(sched);

    if (RB_EMPTY(&sched->sleep.RB)) {
        return;
    } 

    uint64_t now_us = gettimestamp();
    Proc *proc;
    while ((proc = RB_MIN(ProcRB_sleep, &sched->sleep.RB))) {
        if (proc->sleep_us > now_us) {
            break;
        }
        scheduler_remsleep(proc);
        scheduler_addready(proc);
    }
}

static inline
int _scheduler_running(Scheduler *sched)
{
    ASSERT_NOTNULL(sched);

    if (!TAILQ_EMPTY(&sched->readyQ)) {
        return 1;
    }
    
    Proc *proc;
    uint64_t min_us = 0;
    if (!RB_EMPTY(&sched->sleep.RB)) {
        proc = RB_MIN(ProcRB_sleep, &sched->sleep.RB);
        if (proc) {
            min_us = proc->sleep_us;
        }
    }
   
    if (!RB_EMPTY(&sched->altsleep.RB)) {
        proc = RB_MIN(ProcRB_altsleep, &sched->altsleep.RB);
        if (proc) {
            min_us = (proc->sleep_us < min_us) ? proc->sleep_us : min_us;
        }
    }

    if (min_us > 0) {
        /* FIXME */
        uint64_t now_us = gettimestamp();
        if (min_us > now_us) {
            usleep((useconds_t)(min_us - now_us));
        }
        return 1;
    }

    return 0;
}

int scheduler_run(void)
{
    Scheduler *sched = scheduler_self();
    Proc *curr_proc;

    while (_scheduler_running(sched)) {
        PDEBUG("This is from scheduler!\n");

        /* wake up sleeping PROC if timeout */
        _scheduler_wakeup(sched);

        /* find next PROC to run */
        /* check ready Q */
        if (!TAILQ_EMPTY(&sched->readyQ)) {
            curr_proc = TAILQ_FIRST(&sched->readyQ);
            TAILQ_REMOVE(&sched->readyQ, curr_proc, readyQ_next);
            goto procFound;
        }

        /* no PROC found, reevaluate sched_running */
        continue;

        /* from here, a PROC is found to resume */
procFound:
        ASSERT_NOTNULL(curr_proc);
        
        sched->curr_proc = curr_proc;
        sched->curr_proc->state = PROC_RUNNING;

        ctx_switch(&sched->ctx, &sched->curr_proc->ctx);
        ctx_madvise(sched->curr_proc);

        switch (sched->curr_proc->state) {
        case PROC_RUNNING:
        case PROC_READY:
            scheduler_addready(sched->curr_proc);
            break;
        case PROC_ENDED: {
            /* FIXME atomic */

            ProcBuild *build = sched->curr_proc->proc_build;
            /*  resolve ProcBuild */
            if (build != NULL) {
                csp_parsebuild((Builder *)build);
            }
                    
            /* cleanup */
            proc_free(sched->curr_proc);
        }
            break;
        case PROC_RUNWAIT:
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

