
#include <stdio.h>
#include <unistd.h>

#include "debug.h"
#include "proxc.h"

int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    PDEBUG("main start\n");

    proxc_start();

    for (int i = 0; i < 10; i++) {
        printf("This is from main!\n");
        sleep(1);
        scheduler_yield();
    }

    proxc_end();
    return 0;
}

