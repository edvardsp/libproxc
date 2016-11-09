
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "util/debug.h"
#include "internal.h"

Chan* chan_create(void)
{
    Chan *chan;

    if (!(chan = malloc(sizeof(Chan)))) {
        PERROR("malloc failed for Chan\n");
        return NULL;
    }
    memset(chan, 0, sizeof(Chan));
    for (size_t i = 0; i < 2; i++) {
        if (!(chan->ends[i] = malloc(sizeof(ChanEnd)))) {
            PERROR("malloc failed for ChanEnd\n");
            if (chan->ends[0] != NULL)
                free(chan->ends[0]);
            free(chan);
            return NULL;
        }
        memset(chan->ends[i], 0, sizeof(ChanEnd));
    }

    chan->state = CHAN_WAIT;
    
    chan->data = NULL;
    chan->data_size = 0;

    for (size_t i = 0; i < 2; i++) {
        chan->ends[i]->proc = NULL;
        chan->ends[i]->chan = chan;
    }

    return chan;
}

void chan_free(Chan *chan)
{
    if (!chan) return;

    free(chan);
}

ChanEnd* chan_getend(Chan *chan)
{
    ASSERT_NOTNULL(chan);
    
    /* FIXME atomic */
    if (chan->num_ends < 2) {
        PDEBUG("PROC binds to CHANEND\n");
        ChanEnd *chan_end = chan->ends[chan->num_ends++];

        chan_end->proc  = scheduler_self()->curr_proc;
        chan_end->chan  = chan;
        chan_end->guard = NULL;

        return chan_end;
    }
    errno = EPERM;
    PERROR("Two ChanEnds has already been aquired, NULL returned\n");
    return NULL;
}

static 
int _chan_checkbind(ChanEnd *chan_end)
{
    /* 
     * if CHANEND isn't bound to a PROC, or not bound to
     * the running PROC, return error
     */
    #define THIS_PROC chan_end->proc
    #define CURR_PROC chan_end->proc->sched->curr_proc
    if (!THIS_PROC || THIS_PROC != CURR_PROC) {
        errno = EPERM;
        return errno;
    }
    return 0;
}

int chan_trywrite(ChanEnd *chan_end, void *data, size_t size)
{
    ASSERT_NOTNULL(chan_end);

    /* check binding of CHANEND, return errno if illegal */
    if (_chan_checkbind(chan_end) != 0) {
        errno = EPERM;
        PERROR("CHANEND is not bound to a PROC, do nothing\n");
        return 0;
    }

    Chan *chan = chan_end->chan;
    ASSERT_NOTNULL(chan);

    /* FIXME atomic */
    switch (chan->state) {
    case CHAN_OKWRITE:
        errno = EPERM;
        PERROR("Multiple PROCs trying to write to one CHAN, do nothing\n");
        // FALLTHROUGH
    case CHAN_WAIT:
        /* is not ready, fails */
        return 0; 
    case CHAN_ALTREAD: {
        PDEBUG("in CHAN write, ALT on other end\n");
        Guard *guard = chan->end_wait->guard;
        Alt *alt = guard->alt;

        if (!alt_accept(alt, guard)) {
            chan->state = CHAN_WAIT;
            chan->data = NULL;
            chan->data_size = 0;
            chan->end_wait = NULL;
            return 0;
        }

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

        /* resume waiting PROC on other CHANEND */
        Proc *wait_proc = chan->end_wait->proc;
        wait_proc->state = PROC_READY;
        chan->end_wait = NULL;
        scheduler_addproc(wait_proc);
        return 1;
    }
    case CHAN_OKREAD: {
        PDEBUG("CHAN trywrite, read ready\n");
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

        /* resume waiting PROC on other CHANEND */
        Proc *wait_proc = chan->end_wait->proc;
        wait_proc->state = PROC_READY;
        chan->end_wait = NULL;
        scheduler_addproc(wait_proc);
        return 1;
    }
    }
}

int chan_tryread(ChanEnd *chan_end, void *data, size_t size)
{
    ASSERT_NOTNULL(chan_end);

    /* check binding of CHANEND, return errno if illegal */
    if (_chan_checkbind(chan_end) != 0) {
        errno = EPERM;
        PERROR("CHANEND is not bound to a PROC, do nothing\n");
        return 0;
    }

    Chan *chan = chan_end->chan;
    ASSERT_NOTNULL(chan);

    /* FIXME atomic */
    switch (chan->state) {
    case CHAN_ALTREAD:
    case CHAN_OKREAD:
        errno = EPERM;
        PERROR("Multiple PROCs trying to read to one CHAN, do nothing\n");
        // FALLTHROUGH
    case CHAN_WAIT:
        /* is not ready, fails */
        return 0;
    case CHAN_OKWRITE: {
        PDEBUG("CHAN tryread, write ready\n");
        chan->state = CHAN_WAIT;

        size_t min_size = (size > chan->data_size)
                        ? chan->data_size
                        : size;
        /* only copy over data if is requested by both sides */
        if (min_size > 0) {
            ASSERT_NOTNULL(data);
            ASSERT_NOTNULL(chan->data);
            memcpy(data, chan->data, min_size);
        }

        chan->data = NULL;
        chan->data_size = 0;

        /* resume waiting PROC on other CHANEND */
        Proc *wait_proc = chan->end_wait->proc;
        wait_proc->state = PROC_READY;
        chan->end_wait = NULL;
        scheduler_addproc(wait_proc);
        return 1;
    }
    }
    return 0;
}

void chan_write(ChanEnd *chan_end, void *data, size_t size)
{
    ASSERT_NOTNULL(chan_end);

    /* chan_trywrite checks bind */
    if (chan_trywrite(chan_end, data, size)) {
        return;
    } 

    Proc *proc = chan_end->proc;
    Chan *chan = chan_end->chan;
    ASSERT_NOTNULL(proc);
    ASSERT_NOTNULL(chan);

    if (chan->state == CHAN_WAIT) {
        PDEBUG("CHAN write, wait on read\n");
        chan->state = CHAN_OKWRITE;

        chan->data = data;
        chan->data_size = size;
        
        chan->end_wait = chan_end;

        /* yield, and let other end reschedule this end */
        proc->state = PROC_CHANWAIT;
        proc_yield(proc);
    }
}

void chan_read(ChanEnd *chan_end, void *data, size_t size)
{
    ASSERT_NOTNULL(chan_end);

    /* chan_tryread checks bind */
    if (chan_tryread(chan_end, data, size)) {
        return;
    }

    Proc *proc = chan_end->proc;
    Chan *chan = chan_end->chan;
    ASSERT_NOTNULL(proc);
    ASSERT_NOTNULL(chan);

    if (chan->state == CHAN_WAIT) {
        PDEBUG("CHAN read, wait on write\n");
        chan->state = CHAN_OKREAD;

        chan->data = data;
        chan->data_size = size;

        chan->end_wait = chan_end;

        /* yield, and let other end reschedule this end */
        proc->state = PROC_CHANWAIT;
        proc_yield(proc);
    }
}

void chan_altenable(ChanEnd *chan_end, void *data, size_t size)
{
    ASSERT_NOTNULL(chan_end);

    /* check binding of CHANEND, return errno if illegal */
    if (_chan_checkbind(chan_end) != 0) {
        errno = EPERM;
        PERROR("CHANEND is not bound to this CHAN, do nothing\n");
        return;
    }

    Chan *chan = chan_end->chan;
    ASSERT_NOTNULL(chan);

    switch (chan->state) {
    case CHAN_WAIT:
        PDEBUG("chan_altenable, no PROC on other end\n");
        chan->state = CHAN_ALTREAD;
        chan->data = data;
        chan->data_size = size;
        chan->end_wait = chan_end;
        return;
    case CHAN_OKWRITE: {
        PDEBUG("chan_altenable, found PROC on other end\n");
        Guard *guard = chan_end->guard;
        Alt *alt = guard->alt;
        if (!alt_accept(alt, guard)) {
            return;
        }

        chan->state = CHAN_WAIT;

        size_t min_size = (size > chan->data_size)
                        ? chan->data_size
                        : size;
        /* only copy over data if is requested by both sides */
        if (min_size > 0) {
            ASSERT_NOTNULL(data);
            ASSERT_NOTNULL(chan->data);
            memcpy(data, chan->data, min_size);
        }

        chan->data = NULL;
        chan->data_size = 0;
        
        /* resume waiting PROC on other CHANEND */
        Proc *wait_proc = chan->end_wait->proc;
        wait_proc->state = PROC_READY;
        chan->end_wait = NULL;
        scheduler_addproc(wait_proc);
        return;
    } 
    case CHAN_OKREAD:
    case CHAN_ALTREAD:
        errno = EPERM;
        PERROR("Multiple PROCs trying to read to one CHAN, do nothing\n");
        return;
    }
}

void chan_altdisable(ChanEnd *chan_end)
{
    ASSERT_NOTNULL(chan_end);

    Chan *chan = chan_end->chan;
    ASSERT_NOTNULL(chan);

    if (chan->state == CHAN_ALTREAD) {
        chan->state = CHAN_WAIT;
        chan->data = NULL;
        chan->data_size = 0;
        chan->end_wait = NULL;
    }
}

