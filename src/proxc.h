
#ifndef PROXC_H__
#define PROXC_H__

typedef void (*ProcFxn)(void);

void proxc_start(ProcFxn fxn);
void proxc_end(void);

void* proxc_proc(ProcFxn, ...);
int proxc_par(int, ...);

void proc_yield(void);

#ifndef PROXC_NO_MACRO

#   define PROC(...)  proxc_proc(__VA_ARGS__, NULL)
#   define PAR(...)   proxc_par(0, __VA_ARGS__, NULL)

#endif /* PROXC_NO_MACRO */

#endif /* PROXC_H__ */

