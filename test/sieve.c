
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
    Chan *chlong = CHOPEN(long);
    GO(PROC(generate, chlong));
    long prime;
    for (long i = 0; i < 1000; i++) {
        CHREAD(chlong, &prime, long);
        printf("%ld: %ld\n", i, prime);
        Chan *ch1long = CHOPEN(long);
        GO(PROC(filter, chlong, ch1long, (void *)prime));
        chlong = ch1long;
    }
}

int main(void) 
{
    proxc_start(proxc);
}
