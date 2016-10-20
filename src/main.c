
#include <stdio.h>
#include <unistd.h>

#include "debug.h"
#include "proxc.h"

void fxn1(void *arg) 
{ 
    (void)arg; 
    printf("fxn1: Hello from this side!\n");
    for (int i = 0; i < 4; i++) {
        printf("fxn1: %d\n", i);
        proc_yield();
    }
}
void fxn2(void *arg) 
{ 
    if (arg) {
        printf("fxn2: got arg %d\n", *(int *)arg);
    }
    printf("fxn2: Hello from the other side!\n");
    for (int i = 2; i < 7; i++) {
        printf("fxn2: %d\n", i);
        proc_yield();
    }
}

void fxn3(void *arg)
{
    (void)arg;
    printf("fxn3: Now it is my turn!\n");
    for (int i = 0; i < 10; i++) {
        printf("fxn3: %d\n", i);
    }
}

void foofunc(void *arg)
{
    (void)arg;

    printf("foofunc now calls PAR\n");

    int value = 42;

    proxc_PAR(
        proxc_PROC(fxn1),
        proxc_PROC(fxn2, &value),
        proxc_PROC(fxn2),
        proxc_PROC(fxn3)
    );

    printf("foofunc PAR has ended\n");

    PDEBUG("foofunc done\n");
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    PDEBUG("main start\n");

    proxc_start(foofunc);
    
    return 0;
}

