
#ifndef PROXC_H__
#define PROXC_H__

/* FIXME redefined typedef from internal.h */
#ifndef INTERNAL_H__
typedef void (*ProcFxn)(void *);

typedef struct {
    ProcFxn  fxn;
    void     *arg;
} FxnArg;
#endif 

void proxc_start(ProcFxn fxn);
void proxc_end(void);

FxnArg proxc_proc(ProcFxn fxn, void *arg);
int proxc_par(int, ...);

#define proxc_PROC(fxn, arg)  &(FxnArg){ fxn, arg }
#define proxc_PAR(...)        proxc_par(0, __VA_ARGS__, NULL)

void proc_yield(void);

#endif /* PROXC_H__ */

