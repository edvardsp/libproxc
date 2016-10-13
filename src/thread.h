
#ifndef THREAD_H__
#define THREAD_H__

#include <pthread.h>

#include "internal.h"

typedef void (*ThreadFxn)(void *);

struct FxnArg {
    ThreadFxn  fxn;
    void       *arg;
};

struct Thread {
    unsigned long  core_id;
    pthread_t      kernel_thread;
    struct FxnArg  fxn_arg;
};

typedef struct FxnArg FxnArg;
typedef struct Thread Thread;

int thread_setaffinity(Thread *thread);
int thread_create(Thread **new_thread, unsigned long core_id, FxnArg fxn_arg);

#endif /* THREAD_H__ */

