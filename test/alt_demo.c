
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "proxc.h"

void fxn1(void) 
{ 
    printf("fxn1: start\n");

    Chan *ch = ARGN(0);
    ChanEnd *ch_end = chan_getend(ch);
    
    int running = 1;
    while (running) {
        int value = 42;
        printf("fxn1: trying to write %d\n", value);
        chan_write(ch_end, &value, sizeof(int));
        printf("fxn1: succeded\n");
    }

    printf("fxn1: stop\n");
}

void fxn2(void) 
{ 
    printf("fxn2: start\n");

    Chan *ch = ARGN(0);
    ChanEnd *ch_end = chan_getend(ch);

    int running = 1;
    while (running) {
        int value = 666;
        printf("fxn2: trying to write %d\n", value);
        chan_write(ch_end, &value, sizeof(int));
        printf("fxn2: succeded\n");
    }

    printf("fxn2: stop\n");
}

void fxn3(void)
{
    printf("fxn3: start\n");

    Chan *ch = ARGN(0);
    ChanEnd *ch_end = chan_getend(ch);

    int running = 1;
    while (running) {
        int value = 1337;
        printf("fxn3: trying to write %d\n", value);
        chan_write(ch_end, &value, sizeof(int));
        printf("fxn3: succeded\n");
    }

    printf("fxn3: stop\n");
}

__attribute__((noreturn))
void foofunc(void)
{
    printf("foofunc: PAR start\n");

    #define NUM_CHS 3
    Chan *chs[NUM_CHS];
    ChanEnd *ch_ends[NUM_CHS];
    for (int i = 0; i < NUM_CHS; i++) {
        chs[i] = chan_create();
        ch_ends[i] = chan_getend(chs[i]);  
    }

    GO(PAR(
           PROC(fxn3, chs[2]),
           PROC(fxn2, chs[1]),
           PROC(fxn1, chs[0])
       )
    );

    int x = 2, y = 4;
    int val1, val2, val3;
    for (int i = 0; i < 10; i++) {
        printf("foofunc: round %d\n", i);
        switch(ALT(
            GUARD(i  < y, ch_ends[0], &val1, int),
            GUARD(i  > x, ch_ends[1], &val2, int),
            GUARD(1, ch_ends[2], &val3, int)
        )) {
        case 0: // guard 0
            printf("\tguard 0 accepted, %d\n", val1);
            break;
        case 1: // guard 1
            printf("\tguard 1 accepted, %d\n", val2);
            break;
        case 2: // guard 2
            printf("\tguard 2 accepted, %d\n", val3);
            break;
        }
    }

    for (int i = 0; i < NUM_CHS; i++)
        chan_free(chs[i]);

    printf("foofunc: PAR ended\n");
    exit(0);
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    printf("main start\n");

    proxc_start(foofunc);
    
    return 0;
}

