
#include <stdio.h>
#include <unistd.h>

#include "utils.h"
#include "kt_man.h"

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    log_init("proxc", "slog.cfg", 2, 3, 1);
    log(0, SLOG_DEBUG, "Main start");
        
    ktm_init(pthread_self());

    sleep(2);

    ktm_cleanup();

    
    return 0;
}

