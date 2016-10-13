
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

static pthread_t *g_kernel_threads = NULL;
static Scheduler *g_scheds = NULL;

static unsigned long g_num_cores = 0;

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

int scheduler_addctx(Context *ctx)
{
    ASSERT_NOTNULL(ctx);

    Scheduler *sched = scheduler_self();
    TAILQ_INSERT_TAIL(&sched->readyQ, ctx, readyQ_next);
    return 0;
}

int scheduler_run(void)
{
    Scheduler *sched = scheduler_self();

    int running = 1;
    Context *curr_ctx;
    while (running) {
        curr_ctx = TAILQ_LAST(&sched->readyQ, ContextQ);
        if (curr_ctx == NULL) break;

        sched->curr_ctx = curr_ctx;
        PDEBUG("This is from scheduler!\n");
        ASSERT_0(scheduler_switch(&sched->ctx, &curr_ctx->ctx));
    }

    return 0;
}

void* _scheduler_reschedule()
{

    return NULL;
}

void* _scheduler_mainloop(void *arg)
{
    (void)arg;

    Scheduler *sched;
    ASSERT_0(
        scheduler_create(&sched)
    );

    PDEBUG("mainloop started\n");
    Context *new_context;
    for (;;) {
        // Find next contex to switch to
        new_context = TAILQ_LAST(&sched->readyQ, ContextQ);
        /* FIXME */
        if (new_context == NULL) continue;
        sched->curr_ctx = new_context;
        PDEBUG("This is from scheduler!\n");
        ASSERT_0(scheduler_switch(&sched->ctx, &new_context->ctx));
    }

    return NULL;
}

int _scheduler_detect_cores(void)
{
    long nprocessors_onln = sysconf(_SC_NPROCESSORS_ONLN);
    if (nprocessors_onln == -1) {
        PERROR("sysconf failed\n");
        return errno;
    }
    ASSERT_TRUE(nprocessors_onln > 0);
    g_num_cores = (unsigned long)nprocessors_onln;
    PDEBUG("%lu cores detected!\n", g_num_cores);
    return 0;
}

int _scheduler_start_multicore(void)
{
    if ((g_kernel_threads = malloc(sizeof(pthread_t) * g_num_cores)) == NULL) {
        PERROR("malloc failed for g_kernel_threads\n");
        return errno;
    }

    if ((g_scheds = malloc(sizeof(Scheduler) * g_num_cores)) == NULL) {
        PERROR("malloc failed for g_schedulers\n");
        return errno;
    }
    
    // main thread is one of the pthreads
    g_kernel_threads[0] = pthread_self();

    // create rest of the pthreads
    for (unsigned long i = 1; i < g_num_cores; i++) {
        pthread_t *pt = &g_kernel_threads[i];
        ASSERT_0(pthread_create(pt, NULL, _scheduler_mainloop, (void *)i));
    }

    // specify core affinity for each pthread
    cpu_set_t cpuset;
    for (unsigned long i = 0; i < g_num_cores; i++) {
        pthread_t pt = g_kernel_threads[i];
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        ASSERT_0(
            pthread_setaffinity_np(pt, sizeof(cpuset), &cpuset)
        );
    }

    return 0;
}

int _scheduler_start_singlecore(void)
{
    if ((g_kernel_threads = malloc(sizeof(pthread_t))) == NULL) {
        PERROR("malloc failed for g_kernel_threads\n");
        return errno;
    }

    if ((g_scheds = malloc(sizeof(Scheduler))) == NULL) {
        PERROR("malloc failed for g_schedulers\n");
        return errno;
    }

    // main thread is only pthread
    g_kernel_threads[0] = pthread_self();

    return 0;
}

int _scheduler_start(void)
{
    _scheduler_key_create();
    
    int ret;
    // set to multicore
    if (g_num_cores != 0) {
        if ((ret = _scheduler_start_multicore()) != 0) {
            return ret;
        } 
    }
    // or single core
    else {
        if ((ret = _scheduler_start_singlecore()) != 0) {
            return ret;
        }
    }

    return 0;
}

