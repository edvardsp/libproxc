
#include <stdio.h>
#include <time.h>

#include <proxc.h>

#define NUM_WORKERS    10000L
#define NUM_CTXSWITCH  10000L

void worker(void)
{
    for (;;) {
        YIELD();
    }
}

void foofunc(void)
{
    clock_t start, stop;    
    for (long i = 0; i < NUM_WORKERS; ++i)
        GO( PROC(worker) );

    start = clock();
    for (long i = 0; i < NUM_CTXSWITCH; ++i) {
        YIELD();
    }
    stop = clock();
    double diff = (double)(stop - start) / CLOCKS_PER_SEC;
    printf("Num workers: %ld\n", NUM_WORKERS);
    printf("Num ctx switches: %ld\n", NUM_CTXSWITCH);
    printf("Time elapsed: %.2fms\n", diff * 1000.0);
    printf("Time / ctx switch: %.2fns\n", diff * 1000.0 * 1000.0 * 1000.0 / ((NUM_WORKERS + 1)* NUM_CTXSWITCH));
}

int main(void)
{
    proxc_start(foofunc);
    return 0;
}

