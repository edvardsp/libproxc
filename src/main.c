
#include <stdio.h>
#include <unistd.h>

#include "debug.h"
#include "proxc.h"

void foofunc(void)
{
    for (int i = 0; i < 10; i++) {
        PDEBUG("This is from foofunc!\n");
        sleep(1);
        /* scheduler_yield(); */
    }
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    PDEBUG("main start\n");

    proxc_start(foofunc);

    sleep(1);
    
    return 0;
}

