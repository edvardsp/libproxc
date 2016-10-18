
#include <stdio.h>
#include <unistd.h>

#include "debug.h"
#include "proxc.h"

void foofunc(void)
{
    for (int i = 0; i < 4; i++) {
        PDEBUG("This is from foofunc!\n");
        //sleep(1);
        scheduler_yield();
    }
    PDEBUG("foofunc done\n");
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    PDEBUG("main start\n");

    printf("int %zu\n, void* %zu\n", sizeof(int), sizeof(void*));

    proxc_start(foofunc);

    sleep(1);
    
    return 0;
}

