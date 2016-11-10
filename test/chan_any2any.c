
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "proxc.h"

enum {
    NUM_WRITERS = 7,
    NUM_READERS = 3,
    NUM_CHANOPS = 300000
};

void writer(void) 
{ 
    Chan *ch = ARGN(0);
    long val = (long)(void *)ARGN(1);
    
    printf("writer %ld: start\n", val);

    int running = 1;
    while (running) {
        CHWRITE(ch, &val, long);
    }
}

void reader(void)
{
    Chan *ch = ARGN(0);
    long id = (long)(void *)ARGN(1);

    printf("reader %ld: start\n", id);

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
    printf("foofunc: start\n");

    Chan *ch = CHOPEN(long);

    for (size_t i = 0; i < NUM_WRITERS; i++)
        GO(PROC(writer, ch, (void *)i));

    for (size_t i = 0; i < NUM_READERS - 1; i++)
        GO(PROC(reader, ch, (void *)i)); 

    RUN(PROC(reader, ch, (void *)(NUM_READERS - 1)));

    CHCLOSE(ch);

    printf("foofunc: PAR ended\n");
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    printf("main start\n");

    proxc_start(foofunc);
    
    return 0;
}

