
#ifndef CHAN_H__
#define CHAN_H__

#include <stddef.h>
#include <stdint.h>

#include "internal.h"

enum ChanState {
    CHAN_WAIT = 0,  /* no PROC using the CHAN */
    CHAN_WREADY,    /* a PROC is trying to write on CHAN */
    CHAN_RREADY     /* a PROC is trying to read from CHAN */
};

struct Chan {
    enum ChanState  state;

    void    *data;
    size_t  data_size;

    size_t num_ends;
    struct ChanEnd  *ends[2];
    struct ChanEnd  *end_wait;
};

struct ChanEnd {
    struct Proc  *proc;
    struct Chan  *chan;
};

#endif /* CHAN_H__ */

