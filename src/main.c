
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "proxc.h"

void fxn(void) 
{ 
    Chan *ch = ARGN(0);
    long id = (long)(long *)ARGN(1);
    
    printf("fxn%ld: start\n", id);

    int running = 1;
    while (running) {
        long value = id;
        printf("fxn%ld: trying to write %ld\n", id, value);
        CHWRITE(ch, &value, sizeof(long));
        printf("fxn%ld: succeded\n", id);
    }

    printf("fxn%ld: stop\n", id);
}

__attribute__((noreturn))
void foofunc(void)
{
    printf("foofunc: start\n");

    enum { NUM_WRITERS = 3 };

    Chan *chs[NUM_WRITERS];
    for (long i = 0; i < NUM_WRITERS; i++) {
        chs[i] = CHOPEN(long);
        GO(PROC(fxn, chs[i], (void *)i));
    }

    int x = 2, y = 4;
    long val1, val2;
    for (int i = 0; i < 10; i++) {
        printf("foofunc: round %d\n", i);
        switch(ALT(
            GUARD(i  < y, chs[0], &val1, long),
            GUARD(i  > x, chs[1], &val2, long),
            GUARD(1,      chs[2], &val1, long)
        )) {
        case 0: // guard 0
            printf("\tguard 0 accepted, %ld\n", val1);
            break;
        case 1: // guard 1
            printf("\tguard 1 accepted, %ld\n", val2);
            break;
        case 2: // guard 2
            printf("\tguard 2 accepted, %ld\n", val2);
            break;
        }
    }

    for (int i = 0; i < NUM_WRITERS; i++)
        CHCLOSE(chs[i]);

    printf("foofunc: ended\n");
    exit(0);
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    printf("main start\n");

    proxc_start(foofunc);
    
    return 0;
}

