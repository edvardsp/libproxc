
/*************************************
 * Includes
 ************************************/

#include <stddef.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>
#include <ucontext.h>

#include "utils.h"
#include "kt_man.h"

/*************************************
 * Typedefs
 ************************************/

typedef struct {
    ucontext_t ctx;    
} ProcessContext;

typedef struct {
    long core_id;
    ProcessContext *ctx;
} Core;

typedef struct {
    long thread_id;
    Core *curr_core;
    pthread_t kernel_thread;
} Thread;

typedef struct {
    long num_cores;
    Core *cores;
    long num_threads;
    Thread *thread_pool;
} ThreadManager;

/*************************************
 * Global variables
 ************************************/

static ThreadManager thread_manager;

/*************************************
 * Static functions
 ************************************/

static void* workerThread(void* arg)
{
    if (!arg) {
        log(1, SLOG_ERROR, "arg to workerThread is NULL");
        exit(1);
    }

    Thread *thread = (Thread *)arg;
    log(1, SLOG_DEBUG, "workerThread %ld started", thread->thread_id);

    /* context switch goes here */
    for (;;) {}

    return NULL;
}

/*************************************
 * Extern functions
 ************************************/

void ktm_init(pthread_t main_thread)
{
    (void)main_thread;
    log(1, SLOG_DEBUG, "KT Manager start");

    memset(&thread_manager, 0, sizeof(ThreadManager));

    // Find number of online cores on target
    long num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_cores < 1) {
        log(1, SLOG_ERROR, "no online cores available");
        exit(1);
    }

    // Alloc core structures
    Core *cores = malloc(sizeof(Core) * (unsigned long)num_cores);
    if (!cores) {
        log(1, SLOG_ERROR, "malloc failed");
        exit(1);
    }
    // Configure core
    for (long i = 0; i < num_cores; i++) {
        cores[i].core_id = i;
    }

    // Alloc kernel threads up to NUM_CORES
    Thread *threads = malloc(sizeof(Thread) * (unsigned long)num_cores);
    if (!threads) {
        log(1, SLOG_ERROR, "malloc failed");
        exit(1);
    }
    // And copy over main thread to first thread
    threads[0].kernel_thread = main_thread;

    // Configure threads
    for (long i = 0; i < num_cores; i++) {
        threads[i].thread_id = i;
        threads[i].curr_core = &cores[i];
    }

    log(1, SLOG_DEBUG, "KT Manager found %ld cores", num_cores);
    thread_manager.num_cores = num_cores;
    thread_manager.cores = cores;
    thread_manager.num_threads = num_cores;
    thread_manager.thread_pool = threads;
    
    // Start each WorkerThread for each core
    int ret;
    for (long i = 0; i < num_cores; i++) {
        Thread *thread = &threads[i];
        pthread_t kernel_thread = thread->kernel_thread;

        // index 0 is main thread, so no need to create pthread
        if (i != 0) {
            ret = pthread_create(&kernel_thread, NULL, workerThread, thread);
            if (ret != 0) {
                log(1, SLOG_ERROR, "pthread_create failed");
                exit(1);
            }
        }

        // Set thread to core
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        ret = pthread_setaffinity_np(kernel_thread, sizeof(cpuset), &cpuset);
        if (ret != 0) {
            log(1, SLOG_ERROR, "pthread_setaffinity_np failed");
            exit(1);
        }
    }
    
}

void ktm_cleanup(void)
{
    log(1, SLOG_DEBUG, "KT Manager cleanup");
    thread_manager.num_cores = 0;
    free(thread_manager.cores);
    thread_manager.cores = NULL;
    thread_manager.num_threads = 0;
    free(thread_manager.thread_pool);
    thread_manager.thread_pool = NULL;
}

