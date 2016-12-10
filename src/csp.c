
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "internal.h"

void* csp_create(enum BuildType type) {
    Builder *builder = NULL;
    switch (type) {
    case PROC_BUILD: {
        ProcBuild *pb = malloc(sizeof(ProcBuild));
        builder = BUILDER_CAST(pb, Builder*);
        break;
    } 
    case PAR_BUILD: {
        SeqBuild *sb = malloc(sizeof(ParBuild));
        builder = BUILDER_CAST(sb, Builder*);
        break;
    }
    case SEQ_BUILD: {
        ParBuild *pb = malloc(sizeof(SeqBuild)); 
        builder = BUILDER_CAST(pb, Builder*);
        break;
    }
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
        ProcBuild *proc_build = BUILDER_CAST(build, ProcBuild*);
        scheduler_addready(proc_build->proc);
        break;
    }
    case PAR_BUILD: {
        PDEBUG("PAR_BUILD started\n");
        ParBuild *par_build = BUILDER_CAST(build, ParBuild*);
        Builder *child;
        TAILQ_FOREACH(child, &par_build->childQ, header.node) {
            csp_runbuild(child);
        }
        break;
    }
    case SEQ_BUILD: {
        PDEBUG("SEQ_BUILD started\n");
        SeqBuild *seq_build = BUILDER_CAST(build, SeqBuild*);
        csp_runbuild(seq_build->curr_build);
        break;
    }
    }
}

void csp_cleanupbuild(Builder *build)
{
    ASSERT_NOTNULL(build);

    Builder *child;
    switch (build->header.type) {
    case PROC_BUILD:
        PDEBUG("proc_build cleanup\n");
        /* proc_build has no childs, and the scheduler frees */
        /* the actual PROC struct, so only free the proc_build */
        break;
    case PAR_BUILD: {
        PDEBUG("par_build cleanup\n");
        /* recursively free all childs */
        ParBuild *par_build = BUILDER_CAST(build, ParBuild*);
        while (!TAILQ_EMPTY(&par_build->childQ)) {
            child = TAILQ_FIRST(&par_build->childQ);
            TAILQ_REMOVE(&par_build->childQ, child, header.node);
            csp_cleanupbuild(child);
        }
        break;
    }
    case SEQ_BUILD: {
        PDEBUG("seq_build cleanup\n");
        SeqBuild *seq_build = BUILDER_CAST(build, SeqBuild*);
        while (!TAILQ_EMPTY(&seq_build->childQ)) {
            child = TAILQ_FIRST(&seq_build->childQ);
            TAILQ_REMOVE(&seq_build->childQ, child, header.node);
            csp_cleanupbuild(child);
        }
        break;
    }
    }

    /* when childs are freed, free itself */
    csp_free(build);
}

void csp_parsebuild(Builder *build)
{
    ASSERT_NOTNULL(build);
    /* FIXME atomic */

    int build_done = 0;
    switch (build->header.type) {
    case PROC_BUILD: {
        /* a finished proc_build always causes cleanup */
        PDEBUG("proc_build finished\n");
        build_done = 1;
        break;
    }
    case PAR_BUILD: {
        ParBuild *par_build = BUILDER_CAST(build, ParBuild*);
        /* if par_build has no more active childs, do cleanup */
        if (--par_build->num_childs == 0) {
            PDEBUG("par_build finished\n");
            build_done = 1;
            break;
        }
        /* if num_childs != 0 means still running childs */
        break;
    }
    case SEQ_BUILD: {
        SeqBuild *seq_build = BUILDER_CAST(build, SeqBuild*);
        if (--seq_build->num_childs == 0) {
            PDEBUG("seq_build finished\n");
            build_done = 1;
            break;
        }

        /* run next build in SEQ list */
        Builder *cbuild= TAILQ_NEXT(seq_build->curr_build, header.node);
        ASSERT_NOTNULL(cbuild);
        csp_runbuild(cbuild);
        seq_build->curr_build = cbuild;
        break;
    }
    }

    if (build_done) {
        /* if root, then cleanup entire tree */
        if (build->header.is_root) {
            /* if run_proc defined, then root of build  */
            /* is in a RUN, else in GO, then no need */
            /* to reschedule anything */
            Proc *run_proc = build->header.run_proc;
            if (run_proc != NULL) {
                scheduler_addready(run_proc);
            }
            csp_cleanupbuild(build);
        }
        /* or schedule the underlying parent */
        else  {
            Builder *parent = build->header.parent;
            csp_parsebuild(parent);
        }
    }
}
