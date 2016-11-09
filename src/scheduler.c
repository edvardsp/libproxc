
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sched.h>
#include <unistd.h>
#include <ucontext.h>
#include <pthread.h>

#include "util/debug.h"
#include "util/queue.h"
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
    if (!(sched = malloc(sizeof(Scheduler)))) {
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
    TAILQ_INIT(&sched->altQ);
    RB_INIT(&sched->sleepRB);

    *new_sched = sched;

    return 0;
}

void scheduler_free(Scheduler *sched)
{
    if (!sched) return;

    /* FIXME cleanup Proc's in Qs */
    memset(sched, 0, sizeof(Scheduler));
    free(sched);
}

void scheduler_addproc(Proc *proc)
{
    ASSERT_NOTNULL(proc);

    PDEBUG("scheduler_addproc called\n");
       
    Scheduler *sched = proc->sched;
    if (proc->state == PROC_READY) {
        TAILQ_INSERT_TAIL(&sched->readyQ, proc, readyQ_next);
    }
    else if (proc->state == PROC_ALTWAIT) {
        /* FIXME */ 
    }
}

static
void _scheduler_cleanupbuild(Builder *build)
{
    ASSERT_NOTNULL(build);
    
    Builder *child;
    switch (build->header.type) {
    case PROC_BUILD: 
        PDEBUG("proc_build cleanup\n");
        /* proc_build has no childs, and the scheduler frees */
        /* the actual PROC struct, so only free the proc_build */
        break;
    case PAR_BUILD: {
        PDEBUG("par_build cleanup\n");
        /* recursively free all childs */
        ParBuild *par_build = (ParBuild *)build;
        TAILQ_FOREACH(child, &par_build->childQ, header.node) {
            _scheduler_cleanupbuild(child);
        }
        break;
    }
    case SEQ_BUILD: {
        PDEBUG("seq_build cleanup\n");
        SeqBuild *seq_build = (SeqBuild *)build;
        TAILQ_FOREACH(child, &seq_build->childQ, header.node) {
            _scheduler_cleanupbuild(child);
        }
        break;
    }
    } 
     
    /* when childs are freed, free itself */
    csp_free(build);
}

static
void _scheduler_parsebuild(Builder *build)
{
    ASSERT_NOTNULL(build);
    /* FIXME atomic */

    int build_done = 0;
    switch (build->header.type) {
    case PROC_BUILD: {
        /* a finished proc_build always causes cleanup */
        PDEBUG("proc_build finished\n");
        build_done = 1;
        break;
    }
    case PAR_BUILD: {
        ParBuild *par_build = (ParBuild *)build;
        /* if par_build has no more active childs, do cleanup */
        if (--par_build->num_childs == 0) {
            PDEBUG("par_build finished\n");
            build_done = 1;
            break;
        }
        /* if num_childs != 0 means still running childs */
        break;
    }
    case SEQ_BUILD: {
        SeqBuild *seq_build = (SeqBuild *)build;
        if (--seq_build->num_childs == 0) {
            PDEBUG("seq_build finished\n");
            build_done = 1;
            break;
        }

        /* run next build in SEQ list */
        Builder *cbuild= TAILQ_NEXT(seq_build->curr_build, header.node);
        ASSERT_NOTNULL(cbuild);
        csp_runbuild(cbuild);
        seq_build->curr_build = cbuild; 
        break;
    }
    }

    if (build_done) {
        /* if root, then cleanup entire tree */
        if (build->header.is_root) {
            /* if run_proc defined, then root of build  */
            /* is in a RUN, else in GO, then no need */ 
            /* to reschedule anything */
            Proc *run_proc = build->header.run_proc;
            if (run_proc != NULL) {
                run_proc->state = PROC_READY;
                scheduler_addproc(run_proc);
            }
            _scheduler_cleanupbuild(build);
        }
        /* or schedule the underlying parent */
        else  {
            Builder *parent = build->header.parent;
            _scheduler_parsebuild(parent);
        }
    }
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
            /* FIXME atomic */

            ProcBuild *build = sched->curr_proc->proc_build;
            /*  resolve ProcBuild */
            if (build != NULL) {
                _scheduler_parsebuild((Builder *)build);
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

