
#ifndef CHAN_H__
#define CHAN_H__

#include <stddef.h>
#include <stdint.h>

#include "internal.h"

enum ChanType {
    CHAN_WRITE_T = 0,
    CHAN_READ_T,
    CHAN_ALTREAD_T
};

enum ChanState {
    CHAN_WAIT = 0,  /* no PROC using the CHAN */
    CHAN_WREADY,    /* a PROC is trying to write on CHAN */
    CHAN_RREADY     /* a PROC is trying to read from CHAN */
};

struct Chan {
    enum ChanType   type;
    enum ChanState  state;

    void    *data;
    size_t  data_size;

    Proc  *proc_wait;
};

#endif /* CHAN_H__ */

