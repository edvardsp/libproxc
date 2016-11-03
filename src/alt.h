
#ifndef ALT_H__
#define ALT_H__

#include <stddef.h>

#include "internal.h"

struct Guard {
    int key;

    Alt      *alt;
    ChanEnd  *ch_end;

    struct {
        void    *ptr;
        size_t  size;
    } data;

    TAILQ_ENTRY(Guard)  node;
};

struct Alt {
    int  key_count;

    int    is_accepted;
    Guard  *accepted;
    
    struct {
        size_t         num;
        struct GuardQ  Q;
    } guards;
};

#endif /* ALT_H__ */

