
#ifndef ALT_H__
#define ALT_H__

#include <stddef.h>
#include <stdint.h>

#include "internal.h"

struct Guard {
    enum GuardType  type;

    int  key;
    Alt  *alt;
    TAILQ_ENTRY(Guard)  node;
    RB_ENTRY(Guard)     sleepRB_node;

    /* Timer Guard */  
    uint64_t  usec;

    /* Chan Guard */
    Chan     *chan;
    ChanEnd  ch_end;
    struct {
        void    *ptr;
        size_t  size;
    } data;
    int  in_chan;
};

struct Alt {
    int  key_count;

    int    is_accepted;
    Guard  *winner;

    struct {
        int   num;
        Guard **guards;
    } ready;
    
    struct {
        size_t        num;
        struct GuardQ Q;
    } guards;

    /* these only need to exist one of */
    /* as well as guard_time consist of lowest usec */
    Guard  *guard_skip;
    Guard  *guard_time;

    Proc  *proc;
};

#endif /* ALT_H__ */

