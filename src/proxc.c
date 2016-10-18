
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <pthread.h>

#include "debug.h"
#include "internal.h"

static 
int _proxc_setaffinity(unsigned long core_id)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

static
void* _proxc_pthreadfxn(void *arg)
{
    unsigned long core_id = (unsigned long)arg;

    // Set pthread affinity
    ASSERT_0(_proxc_setaffinity(core_id));

    Scheduler *sched;
    scheduler_create(&sched);
    /* FIXME put scheduler on wait */

    return NULL;
}

void proxc_start(ProcFxn fxn)
{
    /* find number of online cores on architecture */
    long nprocessors_onln = sysconf(_SC_NPROCESSORS_ONLN);
    if (nprocessors_onln < 0) {
        PERROR("sysconf failed, default to 1\n");
        nprocessors_onln = 1;
    }
    PDEBUG("Found %ld online cores\n", nprocessors_onln);

    /* spawn pthreads for the other avaiable cores */
    pthread_t *pthreads = NULL;
    if (nprocessors_onln > 1) {
        unsigned long num_pthreads = (unsigned long)(nprocessors_onln - 1);
        if ((pthreads = malloc(sizeof(pthread_t) * num_pthreads)) == 0) {
            PERROR("malloc failed for pthread_t\n");
            exit(EXIT_FAILURE);
        }
        for (unsigned long i = 0; i < num_pthreads; i++) {
            /* core id, is +1 because id=0 is this pthread */
            void *arg = (void *)(i + 1);
            ASSERT_0(pthread_create(&pthreads[i], NULL, _proxc_pthreadfxn, arg));
        }
    }
    
    /* call pthreadfxn as if this pthread just spawned */
    ASSERT_0(_proxc_setaffinity(0));
    Scheduler *sched;
    scheduler_create(&sched);
    Proc *proc;
    proc_create(&proc, fxn);
    scheduler_addproc(proc);
    scheduler_run();

    /* when returned from _proxc_pthreadfxn, assume program to be finished */

    if (pthreads != NULL) {
        /* FIXME cleanup other pthreads */
    }

    /* FIXME cleanup this pthread */

}

