
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "util/debug.h"
#include "internal.h"

Chan* chan_create(size_t data_size)
{
    Chan *chan;

    /* alloc CHAN struct */
    if (!(chan = malloc(sizeof(Chan)))) {
        PERROR("malloc failed for Chan\n");
        return NULL;
    }

    PDEBUG("CHAN of type size %zu created\n", data_size);

    /* set CHAN members */
    chan->data_size = data_size;
    TAILQ_INIT(&chan->endQ);

    return chan;
}

void chan_free(Chan *chan)
{
    if (!chan) return;

    PDEBUG("CHAN closed\n");
    free(chan);
}

int chan_write(Chan *chan, void *data, size_t size)
{
    ASSERT_NOTNULL(chan);
    ASSERT_EQ(size, chan->data_size);

    // << acquire lock <<
    
    ChanEnd *first = TAILQ_FIRST(&chan->endQ);

    /* if chanQ not empty and contains readers */
    if (first && first->type == CHAN_READER) {
        TAILQ_REMOVE(&chan->endQ, first, node);

        // >> release lock >>

        PDEBUG("CHAN write, reader found\n");
        
        /* copy over data */
        memcpy(first->data, data, size);

        /* resume reader */
        first->proc->state = PROC_READY;
        scheduler_addproc(first->proc);
        return 1;
    }

    /* if not, chanQ is empty or contains writers, enqueue self */
    Proc *proc = proc_self();
    struct ChanEnd reader_end = {
        .type  = CHAN_WRITER,
        .data  = data,
        .chan  = chan,
        .proc  = proc,
        .guard = NULL
    };
    TAILQ_INSERT_TAIL(&chan->endQ, &reader_end, node);

    // >> release lock >>
    
    PDEBUG("CHAN write, no readers, enqueue\n");

    /* yield until reader reschedules this end */
    proc->state = PROC_CHANWAIT;
    proc_yield(proc);
    /* here, chan operation is complete */
    return 1;
}

int chan_read(Chan *chan, void *data, size_t size)
{
    ASSERT_NOTNULL(chan);
    ASSERT_EQ(size, chan->data_size); 

    // << acquire lock <<

    ChanEnd *first = TAILQ_FIRST(&chan->endQ);

    /* if chanQ not empty and contains writers */
    if (first && first->type == CHAN_WRITER) {
        TAILQ_REMOVE(&chan->endQ, first, node);

        // >> release lock >>
        
        PDEBUG("CHAN read, writer found\n");
        
        /* copy over data */
        memcpy(data, first->data, size);

        /* resume writer */
        first->proc->state = PROC_READY;
        scheduler_addproc(first->proc);
        return 1;
    }
    
    /* if not, chanQ is empty or contains readers, enqueue self */
    Proc *proc = proc_self();
    struct ChanEnd writer_end = {
        .type  = CHAN_READER,
        .data  = data,
        .chan  = chan,
        .proc  = proc,
        .guard = NULL
    };
    TAILQ_INSERT_TAIL(&chan->endQ, &writer_end, node);
    
    // >> release lock >>

    PDEBUG("CHAN read, no writers, enqueue\n");

    /* yield until writer reschedules this end */
    proc->state = PROC_CHANWAIT;
    proc_yield(proc);
    /* here, chan operation is complete */
    return 1;
}

int chan_altread(Chan *chan, void *data, size_t size)
{
    ASSERT_NOTNULL(chan);
    ASSERT_NOTNULL(data);
    ASSERT_EQ(size, chan->data_size);

    return 0;

    // acquire lock <<
    
    // if chanQ not empty and contains writers then
        // if accepted then
            // pop chanQ
            // release lock >>
            // writer over from writer
            // resume writer
            // return, continue self
        // else
            // release lock >>
            // return, continue self
    
    // enqueue in altQ
    // release lock >>
}

