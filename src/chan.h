
#ifndef CHAN_H__
#define CHAN_H__

#include <stddef.h>
#include <stdint.h>

#include "internal.h"

enum ChanState {
    CHAN_NOPROCS = 0,  /* no PROCs using the CHAN */
    CHAN_WPROCS,       /* some PROCs are trying to write on CHAN */
    CHAN_RPROCS        /* some PROCs are trying to read from CHAN */
};

struct Chan {
    uint64_t        id; 
    enum ChanState  state;

    size_t        num_procs;
    struct ProcQ  chanQ;  /* serves as both writer and reader Q */
};

#endif /* CHAN_H__ */

