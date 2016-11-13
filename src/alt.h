
#ifndef ALT_H__
#define ALT_H__

#include <stddef.h>

#include "internal.h"

struct Guard {
    int key;

    Alt   *alt;
    Chan  *chan;

    ChanEnd  ch_end;

    struct {
        void    *ptr;
        size_t  size;
    } data;

    TAILQ_ENTRY(Guard)  node;
};

struct Alt {
    int  key_count;

    int  is_accepted;
    int  key_accept;
    
    struct {
        size_t         num;
        struct GuardQ  Q;
    } guards;

    Proc  *alt_proc;
};

#endif /* ALT_H__ */

