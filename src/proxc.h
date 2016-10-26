
#ifndef PROXC_H__
#define PROXC_H__

#include <stddef.h>

typedef void (*ProcFxn)(void);

typedef struct Chan Chan;
typedef struct ChanEnd ChanEnd;

int  chan_create(Chan **new_chan);
void chan_free(Chan *chan);
ChanEnd* chan_getend(Chan *chan);
int chan_write(ChanEnd *chan_end, void *data, size_t size);
int chan_read(ChanEnd *chan_end, void *data, size_t size);

void proxc_start(ProcFxn fxn);

void* proxc_proc(ProcFxn, ...);
int   proxc_par(int, ...);
void* proxc_argn(size_t n);
int   proxc_ch_open(int, ...);
int   proxc_ch_close(int, ...);

#ifndef PROXC_NO_MACRO

#   define PROC(...)     proxc_proc(__VA_ARGS__, NULL)
#   define PAR(...)      proxc_par(0, __VA_ARGS__, NULL)
#   define ARGN(index)   proxc_argn(index)
#   define CH_OPEN(...)  proxc_ch_open(0, __VA_ARGS__, NULL)
#   define CH_CLOSE(...) proxc_ch_close(0, __VA_ARGS__, NULL)
#   define CH_END(ch)    chan_getend(ch)

#endif /* PROXC_NO_MACRO */

#endif /* PROXC_H__ */

