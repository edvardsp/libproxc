
#ifndef PROXC_H__
#define PROXC_H__

#include <stddef.h>

#define PROXC_NULL  ((void *)-1)

typedef void (*ProcFxn)(void);

typedef struct Chan Chan;
typedef struct ChanEnd ChanEnd;

typedef struct Builder Builder;

Chan* chan_create(void);
void chan_free(Chan *chan);
ChanEnd* chan_getend(Chan *chan);
void chan_write(ChanEnd *chan_end, void *data, size_t size);
void chan_read(ChanEnd *chan_end, void *data, size_t size);

void proxc_start(ProcFxn fxn);

void* proxc_argn(size_t n);

Builder* proxc_proc(ProcFxn, ...);
Builder* proxc_par(int, ...);
Builder* proxc_seq(int, ...);

int proxc_go(Builder *root);
int proxc_run(Builder *root);

int proxc_ch_open(int, ...);
int proxc_ch_close(int, ...);

#ifndef PROXC_NO_MACRO

#   define ARGN(index)  proxc_argn(index)

#   define PROC(...)  proxc_proc(__VA_ARGS__, PROXC_NULL)
#   define PAR(...)   proxc_par(0, __VA_ARGS__, PROXC_NULL)
#   define SEQ(...)   proxc_seq(0, __VA_ARGS__, PROXC_NULL)

#   define GO(build)   proxc_go(build)
#   define RUN(build)  proxc_run(build)

#   define CH_OPEN(...)   proxc_ch_open(0, __VA_ARGS__, PROXC_NULL)
#   define CH_CLOSE(...)  proxc_ch_close(0, __VA_ARGS__, PROXC_NULL)
#   define CH_END(ch)     chan_getend(ch)
#   define CH_WRITE(ch_end, data, type) \
        chan_write(ch_end, data, sizeof(type)) 
#   define CH_READ(ch_end, data, type) \
        chan_read(ch_end, data, sizeof(type)) 

#endif /* PROXC_NO_MACRO */

#endif /* PROXC_H__ */

