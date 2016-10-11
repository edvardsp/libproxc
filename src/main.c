
#include <stdio.h>
#include <unistd.h>

#include "utils.h"
#include "kt_man.h"

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    log_init("proxc", "slog.cfg", 2, 3, 1);
    log(0, SLOG_DEBUG, "Main start");
        
    static volatile int inInit = 1;
    ucontext_t main_ctx;
    if (getcontext(&main_ctx) == -1) {
        log(0, SLOG_ERROR, "setcontext failed");
        exit(1);
    }
    if (inInit) {
        inInit = 0;
        ktm_init(main_ctx);
    }

    sleep(2);

    ktm_cleanup();

    
    return 0;
}

