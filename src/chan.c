
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "debug.h"
#include "internal.h"

int chan_create(Chan **new_chan)
{
    Chan *chan;

    if ((chan = malloc(sizeof(Chan))) == NULL) {
        PERROR("malloc failed for Chan\n");
        return errno;
    }
    memset(chan, 0, sizeof(Chan));

    chan->type = CHAN_WRITE_T;
    chan->state = CHAN_WAIT;
    
    chan->data = NULL;
    chan->data_size = 0;

    chan->proc_wait = NULL;

    *new_chan = chan;
    return 0;
}

void chan_free(Chan *chan)
{
    if (chan == NULL) return;

    free(chan);
}

void chan_write(Chan *chan, void *data, size_t size)
{
    ASSERT_NOTNULL(chan);

    Scheduler *sched = scheduler_self();
    /* FIXME atomic */
    switch (chan->state) {
    case CHAN_WAIT: 
        chan->state = CHAN_WREADY;

        chan->data = data;
        chan->data_size = size;
        
        chan->proc_wait = sched->curr_proc;

        sched->curr_proc->state = PROC_CHANWAIT;
        break;

    case CHAN_RREADY: {
        chan->state = CHAN_WAIT;

        size_t min_size = (size > chan->data_size) 
                        ? chan->data_size 
                        : size;
        /* only copy over data if is requested by both sides */
        if (min_size > 0) {
            ASSERT_NOTNULL(data);
            ASSERT_NOTNULL(chan->data);
            memcpy(chan->data, data, min_size);
        }

        chan->data = NULL;
        chan->data_size = 0;

        chan->proc_wait->state = PROC_READY;
        scheduler_addproc(chan->proc_wait);
        break;
    }
    case CHAN_WREADY:
        PERROR("Multiple PROCs trying to write to one CHAN, block indefinitely\n");
        sched->curr_proc->state = PROC_ERROR;
        break;
    }

    /* a chan operation always calls a yield */
    proc_yield();
}

void chan_read(Chan *chan, void *data, size_t size)
{
    ASSERT_NOTNULL(chan);

    Scheduler *sched = scheduler_self();
    /* FIXME atomic */
    switch (chan->state) {
    case CHAN_WAIT:
        chan->state = CHAN_RREADY;

        chan->data = data;
        chan->data_size = size;

        chan->proc_wait = sched->curr_proc;

        sched->curr_proc->state = PROC_CHANWAIT;
        break;

    case CHAN_WREADY: {
        chan->state = CHAN_WAIT;

        size_t min_size = (size > chan->data_size) 
                        ? chan->data_size 
                        : size;
        if (min_size > 0) {
            ASSERT_NOTNULL(data);
            ASSERT_NOTNULL(chan->data);
            memcpy(data, chan->data, min_size);
        }

        chan->data = NULL;
        chan->data_size = 0;

        chan->proc_wait->state = PROC_READY;
        scheduler_addproc(chan->proc_wait);
        break;
    }
    case CHAN_RREADY:
        PERROR("Multiple PROCs trying to read from one CHAN, block indefinitely\n");
        sched->curr_proc->state = PROC_ERROR;
        break;
    }

    /* a chan operation always calls a yield */
    proc_yield();
}


