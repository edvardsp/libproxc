
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

    int value = 42;
    chan_write(ch_end, &value, sizeof(int));

    printf("fxn1: stop\n");
}

void fxn2(void) 
{ 
    printf("fxn2: start\n");

    Chan *ch = ARGN(0);
    ChanEnd *ch_end = chan_getend(ch);

    int value = 666;
    chan_write(ch_end, &value, sizeof(int));

    printf("fxn2: stop\n");
}

void fxn3(void)
{
    printf("fxn3: start\n");

    Chan *ch = ARGN(0);
    ChanEnd *ch_end = chan_getend(ch);

    int value = 1337;
    chan_write(ch_end, &value, sizeof(int));

    printf("fxn3: stop\n");
}

void foofunc(void)
{
    printf("foofunc: PAR start\n");

    #define NUM_CHS 3
    Chan *chs[NUM_CHS];
    for (int i = 0; i < NUM_CHS; i++)
        chs[i] = chan_create();

    GO(PAR(
           PROC(fxn1, chs[0]),
           PROC(fxn2, chs[1]),
           PROC(fxn3, chs[2])
       )
    );

    ChanEnd *ch_ends[NUM_CHS];
    for (int i = 0; i < NUM_CHS; i++)
       ch_ends[i] = chan_getend(chs[i]); 

    int x = 2, y = 2;
    int val1, val2, val3;

    switch(ALT(
        GUARD(x  < y, ch_ends[0], &val1, int),
        GUARD(x <= y, ch_ends[1], &val2, int),
        GUARD(x == y, ch_ends[2], &val3, int)
    )) {
    case 0: // guard 0
        printf("guard 0 accepted, %d\n", val1);
        break;
    case 1: // guard 1
        printf("guard 1 accepted, %d\n", val2);
        break;
    case 2: // guard 2
        printf("guard 2 accepted, %d\n", val3);
        break;
    }

    for (int i = 0; i < NUM_CHS; i++)
        chan_free(chs[i]);

    printf("foofunc: PAR ended\n");
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    printf("main start\n");

    proxc_start(foofunc);
    
    return 0;
}

