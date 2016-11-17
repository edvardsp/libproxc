
#include <stdio.h>

#include "proxc.h"

void generate(void)
{
    Chan *chlong = ARGN(0);

    int running = 1;
    long i = 2;
    while (running) {
        CHWRITE(chlong, &i, long);
        i++;
    }
}

void filter(void)
{
    Chan *in_chlong = ARGN(0);
    Chan *out_chlong = ARGN(1);
    long prime = (long)(long *)ARGN(2);
    
    int running = 1;
    long i;
    while (running) {
        CHREAD(in_chlong, &i, long);
        if (i % prime != 0) {
            CHWRITE(out_chlong, &i, long);
        }
    }
}

void proxc(void)
{
    enum { PRIME = 4000 };
    
    Chan *chs[PRIME+1];
    chs[0] = CHOPEN(long);

    Chan *chlong = chs[0];
    GO(PROC(generate, chlong));
    long prime = 0;
    for (long i = 0; i < PRIME; i++) {
        CHREAD(chlong, &prime, long);
        //printf("%ld: %ld\n", i, prime);
        chs[i+1] = CHOPEN(long);
        Chan *new_chlong = chs[i+1];
        GO(PROC(filter, chlong, new_chlong, (void *)prime));
        chlong = new_chlong;
    }
    printf("prime %d: %ld\n", PRIME, prime);

    for (int i = 0; i <= PRIME; i++) {
        CHCLOSE(chs[i]);
    }
}

int main(void) 
{
    proxc_start(proxc);
}
