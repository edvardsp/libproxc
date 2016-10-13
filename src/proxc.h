
#ifndef COROUTINE_H__
#define COROUTINE_H__

typedef void (*ProcFxn)(void);

void proxc_start(ProcFxn fxn);
void proxc_end(void);

void scheduler_yield(void);

#endif /* COROUTINE_H__ */

