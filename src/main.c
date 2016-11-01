
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "proxc.h"

__attribute__((noreturn))
void foofunc(void)
{
    printf("foofunc: start\n");

    Chan *ch1, *ch2;
    CH_OPEN(&ch1, &ch2);
    ChanEnd *ch1_end = CH_END(ch1), *ch2_end = CH_END(ch2);
    int x = 1, y = 2;
    int val1, val2;
    switch(ALT(
        GUARD(x < y, ch1_end, &val1, int),
        GUARD(x == 2, ch2_end, &val2, int)
    )) {
    case 0: // guard 0

        break;
    case 1: // guard 1

        break;
    }
    CH_CLOSE(ch1, ch2);

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

