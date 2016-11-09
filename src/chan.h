
#ifndef CHAN_H__
#define CHAN_H__

#include <stddef.h>
#include <stdint.h>

#include "internal.h"

struct ChanEnd {
    enum {
        CHAN_WRITER,
        CHAN_READER,
        CHAN_ALTER,
    } type;

    struct {
        size_t  size;
        void    *ptr;
    } data;
    
    struct Proc   *proc;
    struct Guard  *guard;

    TAILQ_ENTRY(ChanEnd)  node;
};

struct Chan {
    uint64_t  id;

    size_t  data_size;
    
    struct ChanEndQ  endQ;
};

#endif /* CHAN_H__ */

