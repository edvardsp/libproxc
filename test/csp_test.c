
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "proxc.h"

void fxn1(void) 
{ 
    int val = *(int *)ARGN(0);
    printf("fxn1: start %d\n", val);

    printf("fxn1: stop\n");
}

void fxn2(void) 
{ 
    int val = *(int *)ARGN(0);
    printf("fxn2: start %d\n", val);

    printf("fxn2: stop\n");
}

void fxn3(void)
{
    int val = *(int *)ARGN(0);
    printf("fxn3: start %d\n", val);

    printf("fxn3: stop\n");
}

void fxn4(void)
{
    int val = *(int *)ARGN(0);
    printf("fxn4: start %d\n", val);

    printf("fxn4: stop\n");
}

__attribute__((noreturn))
void foofunc(void)
{
    printf("foofunc: start\n");

    int f = 1, s = 2;

    printf("First RUN\n");
    RUN(PAR(
            PAR(
                SEQ( PROC(fxn1, &f), PROC(fxn2, &f) ),
                PROC(fxn3, &f)
            ),
            SEQ(
                PROC(fxn1, &s),
                PAR( PROC(fxn2, &s), PROC(fxn3, &s) )
            )
        )
    );

    printf("Second RUN\n");
    RUN(PAR(
            PAR(
                SEQ( PROC(fxn1, &f), PROC(fxn2, &f) ),
                PROC(fxn3, &f),
                PROC(fxn4, &f)
            ),
            SEQ(
                PROC(fxn1, &s),
                PROC(fxn2, &s),
                PAR( PROC(fxn3, &s), PROC(fxn4, &s) )
            )
        )
    );

    printf("foofunc: stop\n");
    exit(0);
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    printf("main start\n");

    proxc_start(foofunc);
    
    return 0;
}

