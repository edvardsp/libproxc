
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "proxc.h"

void printer(void)
{
    for (;;) {
        printf("printer: hello!\n");
        SLEEP(MSEC(500));
    }
}

void fxn(void)
{
    int id  = *(int *)ARGN(0);
    int val = *(int *)ARGN(1);
    printf("fxn%d: start, value %d\n", id, val);
    SLEEP(SEC((uint64_t)(1 + rand() % id)));
    printf("fxn%d: stop, value %d\n", id, val);
}

void foofunc(void)
{
    GO( PROC(printer) );

    int ids[3] = { 1, 2, 3 };
    int one = 1, two = 2;

    printf("First RUN\n");
    RUN(PAR(
            PAR(
                SEQ( PROC(fxn, &ids[0], &one), PROC(fxn, &ids[1], &one) ),
                PROC(fxn, &ids[2], &one)
            ),
            SEQ(
                PROC(fxn, &ids[0], &two),
                PAR( PROC(fxn, &ids[1], &two), PROC(fxn, &ids[2], &two) )
            )
        )
    );

    printf("Second RUN\n");
    RUN(PAR(
            PAR(
                SEQ( PROC(fxn, &ids[0], &one), PROC(fxn, &ids[1], &one) ),
                PROC(fxn, &ids[2], &one),
                PROC(fxn, &ids[2], &one)
            ),
            SEQ(
                PROC(fxn, &ids[0], &two),
                PROC(fxn, &ids[0], &two),
                PAR( PROC(fxn, &ids[1], &two), PROC(fxn, &ids[2], &two) )
            )
        )
    );
}

int main(void)
{
    srand((unsigned int)time(0));
    proxc_start(foofunc);
    return 0;
}

