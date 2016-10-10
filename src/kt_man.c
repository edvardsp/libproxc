
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#include "utils.h"
#include "kt_man.h"

typedef struct {
    long core_id;
} Core;

typedef struct {
    long num_cores;
    Core *cores;
    int num_threads;
    pthread_t *thread_pool;
} ThreadManager;

static ThreadManager thread_manager;

void ktm_init(void)
{
    log(1, SLOG_DEBUG, "KT Manager start");

    memset(&thread_manager, 0, sizeof(ThreadManager));

    // Find number of online cores on target
    long num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_cores < 1) {
        log(1, SLOG_ERROR, "number of online cores are below 1");
        exit(1);
    }

    // Alloc core structures
    Core *cores = malloc(sizeof(Core) * (unsigned long)num_cores);
    if (!cores) {
        log(1, SLOG_ERROR, "malloc failed");
        exit(1);
    }

    // Alloc kernel threads for each core
    pthread_t *threads = malloc(sizeof(pthread_t) * (unsigned long)num_cores);
    if (!threads) {
        log(1, SLOG_ERROR, "malloc failed");
        exit(1);
    }

    log(1, SLOG_DEBUG, "KT Manager found %ld cores", num_cores);
    thread_manager.num_cores = num_cores;
    thread_manager.cores = cores;
    thread_manager.num_threads = num_cores;
    thread_manager.thread_pool = threads;
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

