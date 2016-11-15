
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "proxc.h"

void worker(void) 
{ 
    Chan *chint = ARGN(0);
    int id = *(int *)ARGN(1);
    
    printf("worker %d: start\n", id);

    int running = 1;
    int value = id * 3;
    while (running) {
        printf("worker %d: trying to write %d\n", id, value);
        CHWRITE(chint, &value, int);
        printf("worker %d: succeded\n", id);
    }

    printf("worker %d: stop\n", id);
}

__attribute__((noreturn))
void foofunc(void)
{
    printf("foofunc: start\n");

    enum { NUM_WORKERS = 3 };
    Chan *chs[NUM_WORKERS];
    int ids[NUM_WORKERS];
    for (int i = 0; i < NUM_WORKERS; i++) {
        chs[i] = CHOPEN(int);
        ids[i] = i;
        GO(PROC(worker, chs[i], &ids[i]));
    }
    YIELD();

    int x = 2, y = 4;
    int val1, val2, val3;
    for (int i = 0; i < 10; i++) {
        printf("foofunc: round %d\n", i);
        switch(ALT(
            GUARD(i  < y, chs[0], &val1, int),
            GUARD(i  > x, chs[1], &val2, int),
            GUARD(     1, chs[2], &val3, int)
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

    for (int i = 0; i < NUM_WORKERS; i++)
        CHCLOSE(chs[i]);

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

