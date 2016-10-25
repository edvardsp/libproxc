
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "proxc.h"

void fxn1(void) 
{ 
    Chan *a = ARGN(0), *b = ARGN(1);
    int value = 0;
    chan_write(a, &value, sizeof(int));
    int running = 1;
    while (running) {
        chan_read(b, &value, sizeof(int));
        chan_write(a, &value, sizeof(int));
    }
}
void fxn2(void) 
{ 
    Chan *a = ARGN(0), *c = ARGN(1), *d = ARGN(2);
    int value;
    int running = 1;
    while (running) {
        chan_read(a, &value, sizeof(int));
        chan_write(d, &value, sizeof(int));
        chan_write(c, &value, sizeof(int));
    }
}

void fxn3(void)
{
    Chan *b = ARGN(0), *c = ARGN(1);
    int value;
    int running = 1;
    while (running) {
        chan_read(c, &value, sizeof(int));
        value++;
        chan_write(b, &value, sizeof(int));
    }
}

__attribute((noreturn))
void timerFxn(void)
{
    Chan *d = ARGN(0);
    int repeat = 5000;
    int runs = 30;
    clock_t start, stop;
    double time_spent, sum;

    sum = 0;
    for (int j = 0; j < runs; j++) {
        start = clock();

        int x;
        for (int i = 0; i < repeat; i++ ) {
            chan_read(d, &x, sizeof(int));
        }
        stop = clock();
        time_spent = (double)(stop - start) / CLOCKS_PER_SEC * 1000.0;
        sum += time_spent;
        printf("\t%f\n", time_spent);
    }

    printf("Average = %f ms/iteration\n", (sum) / (runs));
    exit(0);
}

void foofunc(void)
{
    printf("foofunc: PAR start\n");

    Chan *a, *b, *c, *d;
    chan_create(&a);
    chan_create(&b);
    chan_create(&c);
    chan_create(&d);
    PAR(
        PROC(fxn1, a, b),
        PROC(fxn2, a, c, d),
        PROC(fxn3, b, c),
        PROC(timerFxn, d)
    );
    chan_free(a);
    chan_free(b);
    chan_free(c);
    chan_free(d);

    printf("foofunc: PAR ended\n");
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    printf("main start\n");

    proxc_start(foofunc);
    
    return 0;
}

