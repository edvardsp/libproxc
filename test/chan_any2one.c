
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "proxc.h"

void fxn1(void) 
{ 
    printf("fxn1: start\n");
    Chan *a = ARGN(0);

    int value = 69;
    int running = 1;
    while (running) {
        printf("fxn1: writing\n");
        CHWRITE(a, &value, int);
    }
}
void fxn2(void) 
{ 
    printf("fxn2: start\n");
    Chan *a = ARGN(0);

    int value = 37;
    int running = 1;
    while (running) {
        printf("fxn2: writing\n");
        CHWRITE(a, &value, int);
    }
}

void fxn3(void)
{
    printf("fxn3: start\n");
    Chan *a = ARGN(0);

    int value = 1000;
    int running = 1;
    while (running) {
        printf("fxn3: writing\n");
        CHWRITE(a, &value, int);
    }
}

void fxn4(void)
{
    printf("fxn4: start\n");
    Chan *a = ARGN(0);

    int value;
    int running = 1;
    while (running) {
        CHREAD(a, &value, int);
        printf("%d\n", value);
    }
}

void foofunc(void)
{
    printf("foofunc: PAR start\n");

    Chan *a = CHOPEN(int);

    RUN(PAR(
        PROC(fxn1, a),
        PROC(fxn2, a),
        PROC(fxn3, a),
        PROC(fxn4, a)
    ));

    CHCLOSE(a);

    printf("foofunc: PAR ended\n");
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    printf("main start\n");

    proxc_start(foofunc);
    
    return 0;
}

