
#ifndef ALT_H__
#define ALT_H__

#include <stddef.h>

#include "internal.h"

struct Guard {
    int pri_case;

    ChanEnd  *ch_end;

    struct {
        void    *ptr;
        size_t  size;
    } data;

    TAILQ_ENTRY(Guard)  node;
};

struct Alt {
    struct {
        size_t         num;
        struct GuardQ  Q;
    } guards;
};

#endif /* ALT_H__ */

