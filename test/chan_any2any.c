
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <proxc.h>

#define NUM_WRITERS 7
#define NUM_READERS 3
#define NUM_CHANOPS 300000l

void writer(void) 
{ 
    Chan *ch = ARGN(0);
    long val = (long)(void *)ARGN(1);

    for (;;) {
        CHWRITE(ch, &val, long);
    }
}

void reader(void)
{
    Chan *ch = ARGN(0);
    long id = (long)(void *)ARGN(1);

    long stats[NUM_WRITERS] = { 0 };

    long value;
    for (size_t i = 0; i < NUM_CHANOPS; i++) {
        CHREAD(ch, &value, long);
        stats[value]++;
    }

    printf("reader %ld: stats\n", id);
    for (size_t i = 0; i < NUM_WRITERS; i++) 
        printf("\twriter %zu: %ld\n", i, stats[i]);
}

void foofunc(void)
{
    Chan *ch = CHOPEN(long);

    for (size_t i = 0; i < NUM_WRITERS; i++)
        GO(PROC(writer, ch, (void *)i));

    for (size_t i = 0; i < NUM_READERS - 1; i++)
        GO(PROC(reader, ch, (void *)i)); 

    RUN(PROC(reader, ch, (void *)(NUM_READERS - 1)));

    CHCLOSE(ch);
}

int main(void)
{
    proxc_start(foofunc);
    return 0;
}

