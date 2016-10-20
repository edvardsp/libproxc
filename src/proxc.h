
#ifndef PROXC_H__
#define PROXC_H__

typedef void (*ProcFxn)(void);

void proxc_start(ProcFxn fxn);
void proxc_end(void);

void* proxc_proc(ProcFxn, ...);
int proxc_par(int, ...);

#define PROC(...)  proxc_proc(__VA_ARGS__, NULL)
#define PAR(...)   proxc_par(0, __VA_ARGS__, NULL)

void proc_yield(void);

#endif /* PROXC_H__ */

