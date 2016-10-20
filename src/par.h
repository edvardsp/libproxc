
#ifndef PAR_H__
#define PAR_H__

#include "internal.h"

struct Par {
    uint64_t  id;
    Proc      *par_proc;

    size_t        num_procs;
    struct ProcQ  joinQ;
};

#endif /* PAR_H__ */

