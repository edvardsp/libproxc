
#include <stdlib.h>
#include <errno.h>
#include <sched.h>
#include <unistd.h>
#include <ucontext.h>
#include <pthread.h>

#include "debug.h"
#include "queue.h"
#include "internal.h"
#include "thread.h"
#include "context.h"

// Holds corresponding scheduler for each pthread
pthread_key_t g_scheduler_key;
ucontext_t main_ctx;

static pthread_t *g_kernel_threads = NULL;
static Scheduler *g_schedulers = NULL;

static unsigned long g_num_cores = 0;

static
void _scheduler_key_destroy(void *data)
{
    Scheduler *scheduler = (Scheduler *)data;
    /* FIXME proper cleanup of scheduler */
    free(scheduler);
    PDEBUG("key_destroy called\n");
}

static
void _scheduler_key_create(void)
{
    ASSERT_0(
        pthread_key_create(&g_scheduler_key, _scheduler_key_destroy)
    );

}

static
int _scheduler_create(unsigned long core_id)
{
    Scheduler *scheduler = &g_schedulers[core_id];

    scheduler->id = core_id;
    scheduler->stack_size = MAX_STACK_SIZE;
    scheduler->current_context = NULL;

    /* FIXME */
    // Save scheduler for this pthread
    ASSERT_0(
        pthread_setspecific(g_scheduler_key, scheduler)
    );
    // and context
    ASSERT_0(
        getcontext(&scheduler->ctx)
    );

    TAILQ_INIT(&scheduler->readyQ);

    return 0;
}

void* _scheduler_reschedule()
{

    return NULL;
}

void* _scheduler_mainloop(void *arg)
{
    unsigned long core_id = (unsigned long)arg;

    if (_scheduler_create(core_id)) {
        PERROR("_scheduler_initalize failed\n");
        return NULL;
    }

    Scheduler *scheduler = scheduler_self();

    if (core_id == 0) {
        Context *context;
        context_create(&context, &main_ctx);
    }

    printf("mainloop started\n");
    Context *new_context;
    for (;;) {
        // Find next contex to switch to
        new_context = TAILQ_LAST(&scheduler->readyQ, ContextQ);
        if (new_context == NULL) continue;
        scheduler->current_context = new_context;
        printf("This is from scheduler!\n");
        ASSERT_0(
            scheduler_switch(&scheduler->ctx, &new_context->ctx)
        );
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
    ASSERT_1(nprocessors_onln > 0);
    g_num_cores = (unsigned long)nprocessors_onln;
    printf("%lu cores detected!\n", g_num_cores);
    return 0;
}

int _scheduler_start_multicore(void)
{
    if ((g_kernel_threads = malloc(sizeof(pthread_t) * g_num_cores)) == NULL) {
        PERROR("malloc failed for g_kernel_threads\n");
        return errno;
    }

    if ((g_schedulers = malloc(sizeof(Scheduler) * g_num_cores)) == NULL) {
        PERROR("malloc failed for g_schedulers\n");
        return errno;
    }
    
    // main thread is one of the pthreads
    g_kernel_threads[0] = pthread_self();

    // create rest of the pthreads
    for (unsigned long i = 1; i < g_num_cores; i++) {
        pthread_t *pt = &g_kernel_threads[i];
        ASSERT_0(
            pthread_create(pt, NULL, _scheduler_mainloop, (void *)i)
        );
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

    if ((g_schedulers = malloc(sizeof(Scheduler))) == NULL) {
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

