
#ifndef PAR_H__
#define PAR_H__

#include <stddef.h>

#include "util/queue.h"
#include "internal.h"

struct Header {
    enum BuildType  type;
    struct Builder  *parent;
    
    int   is_root;
    Proc  *run_proc;

    TAILQ_ENTRY(Builder)  node;
};

struct Builder {
    struct Header header;
};

struct ProcBuild {
    struct Header header;

    Proc  *proc;
};

struct ParBuild {
    struct Header header;

    size_t           num_childs;
    struct BuilderQ  childQ;
};

struct SeqBuild {
    struct Header header;

    size_t   num_childs;
    Builder  *curr_build;
    struct BuilderQ  childQ;
};

#endif /* PAR_H__ */

