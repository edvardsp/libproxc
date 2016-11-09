
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "util/debug.h"
#include "internal.h"

void* csp_create(enum BuildType type) {
    Builder *builder = NULL;
    switch (type) {
    case PROC_BUILD: builder = malloc(sizeof(ProcBuild)); break;
    case PAR_BUILD:  builder = malloc(sizeof(ParBuild));  break;
    case SEQ_BUILD:  builder = malloc(sizeof(SeqBuild));  break;
    }
    if (!builder) return NULL;

    builder->header.type = type;
    builder->header.parent = NULL;
    builder->header.is_root = 0;
    builder->header.run_proc = NULL;

    return builder;
}

void csp_free(Builder *build)
{
    if (!build) return;

    free(build);
}

int csp_insertchilds(size_t *num_childs, Builder *builder, struct BuilderQ *childQ, va_list vargs)
{
    ASSERT_NOTNULL(num_childs);
    ASSERT_NOTNULL(builder);
    ASSERT_NOTNULL(childQ);

    Builder *child = va_arg(vargs, Builder *);
    while (child != PROXC_NULL) {
        /* NULL means failure in child build */
        if (!child) {
            errno = EPERM;
            PERROR("child in csp_insertchilds is NULL\n");
            break;
        }

        /* set parent pointer in child, and insert in builderQ */
        child->header.parent = builder;
        (*num_childs)++;
        TAILQ_INSERT_TAIL(childQ, child, header.node);
        child = va_arg(vargs, Builder *);
    }

    return 0;
}

void csp_runbuild(Builder *build)
{
    switch (build->header.type) {
    case PROC_BUILD: {
        PDEBUG("PROC_BUILD started\n");
        ProcBuild *proc_build = (ProcBuild *)build;
        scheduler_addproc(proc_build->proc);
        break;
    }
    case PAR_BUILD: {
        PDEBUG("PAR_BUILD started\n");
        ParBuild *par_build = (ParBuild *)build;
        Builder *child;
        TAILQ_FOREACH(child, &par_build->childQ, header.node) {
            csp_runbuild(child);
        }
        break;
    }
    case SEQ_BUILD: {
        PDEBUG("SEQ_BUILD started\n");
        SeqBuild *seq_build = (SeqBuild *)build;
        csp_runbuild(seq_build->curr_build);
        break;
    }
    }
}

