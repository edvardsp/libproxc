
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <proxc.h>

void worker(void) 
{ 
    Chan *chint = ARGN(0);
    int id = *(int *)ARGN(1);
    
    printf("worker %d: start\n", id);

    int value = id * 3;
    for (;;) {
        printf("worker %d: trying to write %d\n", id, value);
        CHWRITE(chint, &value, int);
        printf("worker %d: succeded\n", id);
        SLEEP(MSEC(100));
    }

    printf("worker %d: stop\n", id);
}

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
    int val1 = 0, val2 = 0, val3 = 0;
    for (int i = 0; i < 10; i++) {
        printf("foofunc: round %d\n", i);
        switch (ALT(
            CHAN_GUARD(i < y, chs[0], &val1, int),
            CHAN_GUARD(i > x, chs[1], &val2, int),
            CHAN_GUARD(    1, chs[2], &val3, int),
            TIME_GUARD(    1, MSEC(500)),
            SKIP_GUARD(1)
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
        case 3: // guard 3
            printf("\tguard 3 accepted\n");
            break;
        case 4: // guard 4
            printf("\tguard 4 accepted\n");
            break;
        }
        SLEEP(MSEC(333));
    }

    for (int i = 0; i < NUM_WORKERS; i++)
        CHCLOSE(chs[i]);

    printf("foofunc: stop\n");
}

int main(void)
{
    proxc_start(foofunc);
    
    return 0;
}

