
#include <stdlib.h>
#include <errno.h>
#include <sched.h>
#include <unistd.h>
#include <pthread.h>

#include "debug.h"
#include "internal.h"
#include "thread.h"

static
void* _thread_pthreadfxn(void *arg)
{
    ASSERT_NOTNULL(arg);
    struct FxnArg *fxn_arg = (struct FxnArg *)arg;
    fxn_arg->fxn(fxn_arg->arg);
    return NULL;
}

int thread_setaffinity(Thread *thread)
{
    long nprocessors_onln = sysconf(_SC_NPROCESSORS_ONLN);
    if (nprocessors_onln < 0) {
        PERROR("sysconf failed, default nprocessors_onln to 1\n");
        nprocessors_onln = 1;
    }
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    if (thread->core_id < (unsigned long)nprocessors_onln)
        CPU_SET(thread->core_id, &cpuset);
    else
        CPU_SET(0, &cpuset);

    return pthread_setaffinity_np(thread->kernel_thread, sizeof(cpuset), &cpuset);
}

int thread_create(Thread **new_thread, unsigned long core_id, FxnArg fxn_arg)
{
    Thread *thread;
    if ((thread = malloc(sizeof(Thread))) == NULL) {
        PERROR("malloc failed for Thread\n");
        return errno;
    }

    thread->fxn_arg = fxn_arg;

    ASSERT_0(
        pthread_create(&thread->kernel_thread, NULL, _thread_pthreadfxn, &thread->fxn_arg)
    );

    thread->core_id = core_id;
    ASSERT_0(
        thread_setaffinity(thread)
    );

    *new_thread = thread;

    return 0;
}



