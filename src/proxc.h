
#ifndef PROXC_H__
#define PROXC_H__

/* FIXME redefined typedef from internal.h */
#ifndef INTERNAL_H__
typedef void (*ProcFxn)(void *);
#endif 

typedef struct {
    ProcFxn  fxn;
    void     *arg1;
} FxnArg;

void proxc_start(ProcFxn fxn);
void proxc_end(void);

int proxc_par(int, ...);

#define proxc_PROC(...)  PROC_BUILD(__VA_ARGS__, NULL)
#define PROC_BUILD(fxn, arg, ...)  &(FxnArg){ fxn, arg }
#define proxc_PAR(...)  proxc_par(0, __VA_ARGS__, NULL)

void proc_yield(void);

#endif /* PROXC_H__ */

