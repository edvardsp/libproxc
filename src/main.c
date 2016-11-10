
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "proxc.h"

void fxn1(void) 
{ 
    printf("fxn1: start\n");
    Chan *a = ARGN(0), *b = ARGN(1);
    int value = 0;
    CHWRITE(a, &value, int);
    int running = 1;
    while (running) {
        CHREAD(b, &value, int);
        CHWRITE(a, &value, int);
    }
}
void fxn2(void) 
{ 
    printf("fxn2: start\n");
    Chan *a = ARGN(0), *c = ARGN(1), *d = ARGN(2);
    int value;
    int running = 1;
    while (running) {
        CHREAD(a, &value, int);
        CHWRITE(d, &value, int);
        CHWRITE(c, &value, int);
    }
}

void fxn3(void)
{
    printf("fxn3: start\n");
    Chan *b = ARGN(0), *c = ARGN(1);
    int value;
    int running = 1;
    while (running) {
        CHREAD(c, &value, int);
        value++;
        CHWRITE(b, &value, int);
    }
}

__attribute((noreturn))
void timerFxn(void)
{
    Chan *d = ARGN(0);
    int repeat = 10000;
    int runs = 50;
    clock_t start, stop;
    double time_spent, sum;

    printf("Repeat = %d, runs = %d\n", repeat, runs);
    sum = 0;
    for (int j = 0; j < runs; j++) {
        start = clock();

        int x;
        for (int i = 0; i < repeat; i++ ) {
            CHREAD(d, &x, int);
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

    Chan *a = CHOPEN(int);
    Chan *b = CHOPEN(int);
    Chan *c = CHOPEN(int);
    Chan *d = CHOPEN(int);

    RUN(PAR(
        PROC(fxn1,     a, b),
        PROC(fxn2,     a, c, d),
        PROC(fxn3,     b, c),
        PROC(timerFxn, d)
    ));

    CHCLOSE(a);
    CHCLOSE(b);
    CHCLOSE(c);
    CHCLOSE(d);

    printf("foofunc: PAR ended\n");
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    printf("main start\n");

    proxc_start(foofunc);
    
    return 0;
}

