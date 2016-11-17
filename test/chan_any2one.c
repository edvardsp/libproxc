
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "proxc.h"

void writer(void)
{
    Chan *ch = ARGN(0);
    long value = 42;
    for (;;) {
        CHWRITE(ch, &value, int); 
    }
}

void foofunc(void)
{
    Chan *ch = CHOPEN(int);

#define NUM_WRITERS 7l
#define NUM_OPS 50000000l

    for (long i = 0; i < NUM_WRITERS; i++) {
        GO( PROC(writer, ch) );
    }

    YIELD();

    int value;
    clock_t start, stop;
    start = clock();
    for (long i = 0; i < NUM_OPS; i++) {
        CHREAD(ch, &value, int);
    }
    stop = clock();
    double time_ms = (stop - start) * 1000.0 / CLOCKS_PER_SEC;

    printf("Num writers:  %ld\n", NUM_WRITERS);
    printf("Num chanops:  %ld\n", NUM_OPS);
    printf("Time:         %fms\n", time_ms);
    printf("ns/chanread:  %fns\n", time_ms * 1000.0 * 1000.0 / (double)NUM_OPS);

    CHCLOSE(ch);
}

int main(void)
{
    proxc_start(foofunc);
    return 0;
}

