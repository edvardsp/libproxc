
#include <stdio.h>
#include <unistd.h>

#include "utils.h"
#include "kt_man.h"
#include "coroutine.h"

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    log_init("proxc", "slog.cfg", 2, 3, 1);
    log(0, SLOG_DEBUG, "Main start");   

    scheduler_start();

    for (int i = 0; i < 10; i++) {
        printf("This is from main!\n");
        sleep(1);
        scheduler_yield();
    }

    scheduler_end();
    return 0;
}

