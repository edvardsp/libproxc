
#include <stdio.h>
#include <unistd.h>

#include "debug.h"
#include "proxc.h"

void fxn(void *arg) { (void)arg; PDEBUG("fxn()\n"); }
void fxn2(void *arg) { (void)arg; PDEBUG("fxn2()\n"); }

void foofunc(void *arg)
{
    (void)arg;
    for (int i = 0; i < 4; i++) {
        printf("%d\n", i);
        PDEBUG("This is from foofunc!\n");
        proc_yield();
    }

    proxc_PAR(
        proxc_PROC(fxn, NULL),
        proxc_PROC(fxn2, NULL)
    );

    PDEBUG("foofunc done\n");
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    PDEBUG("main start\n");

    proxc_start(foofunc);
    
    return 0;
}

