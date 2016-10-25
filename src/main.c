
#include <stdio.h>
#include <unistd.h>

#include "proxc.h"

void fxn1(void) 
{ 
    Chan *ch1 = ARGN(0);
    printf("fxn1: Hello from this side!\n");
    for (int i = 0; i < 4; i++) {
        int value = i * 11;
        printf("fxn1: to chan, %d\n", value);
        chan_write(ch1, &value, sizeof(int));
    }
}
void fxn2(void) 
{ 
    Chan *ch1 = ARGN(0);
    if (ch1) {
        printf("fxn2: got CHAN!\n");
        for (int i = 0; i < 4; i++) {
            int value;
            chan_read(ch1, &value, sizeof(int));
            printf("fxn2: from chan, %d\n", value);
        }
    }
    else {
        printf("fxn2: no CHAN...\n");
        for (int i = 2; i < 7; i++) {
            printf("fxn2: %d\n", i);
            proc_yield();
        }
    }
}

void fxn3(void)
{
    printf("fxn3: Now it is my turn!\n");
    for (int i = 0; i < 10; i++) {
        printf("fxn3: %d\n", i);
    }

    printf("fxn3: PAR start\n");
    Chan *ch2;
    chan_create(&ch2);
    PAR(
        PROC(fxn2),
        PROC(fxn1, ch2),
        PROC(fxn2, ch2)
    );
    chan_free(ch2);
    printf("fxn3: PAR ended\n");
}

void foofunc(void)
{
    printf("foofunc: PAR start\n");

    Chan *ch1;
    chan_create(&ch1);
    PAR(
        PROC(fxn1, ch1),
        PROC(fxn2, ch1),
        PROC(fxn3),
        PROC(fxn2)
    );
    chan_free(ch1);

    printf("foofunc: PAR ended\n");
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    printf("main start\n");

    proxc_start(foofunc);
    
    return 0;
}

