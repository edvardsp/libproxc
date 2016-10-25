
#ifndef PROXC_H__
#define PROXC_H__

#include <stddef.h>

typedef void (*ProcFxn)(void);

struct Chan;
typedef struct Chan Chan;

int  chan_create(Chan **new_chan);
void chan_free(Chan *chan);
void chan_write(Chan *chan, void *data, size_t size);
void chan_read(Chan *chan, void *data, size_t size);

void proxc_start(ProcFxn fxn);

void* proxc_proc(ProcFxn, ...);
int   proxc_par(int, ...);
void* proxc_argn(size_t n);

void proc_yield(void);

#ifndef PROXC_NO_MACRO

#   define PROC(...)    proxc_proc(__VA_ARGS__, NULL)
#   define PAR(...)     proxc_par(0, __VA_ARGS__, NULL)
#   define ARGN(index)  proxc_argn(index)

#endif /* PROXC_NO_MACRO */

#endif /* PROXC_H__ */

