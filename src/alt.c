
#include <stdlib.h>

#include "internal.h"

Guard* alt_guardcreate(ChanEnd *ch_end, void *out, size_t size)
{
    ASSERT_NOTNULL(ch_end);

    /* alloc GUARD struct */
    Guard *guard;
    if ((guard = malloc(sizeof(Guard))) == NULL) {
        PERROR("malloc failed for GUARD\n");
        return NULL;
    }

    /* set struct members */
    guard->pri_case  = -1;
    guard->ch_end    = ch_end;
    guard->data.ptr  = out;
    guard->data.size = size;

    return guard;
}

void alt_guardfree(Guard *guard)
{
    if (guard == NULL) return;

    free(guard);
}

Alt* alt_create(void)
{
    /* alloc ALT struct */
    Alt *alt;
    if ((alt = malloc(sizeof(Alt))) == NULL) {
        PERROR("malloc failed for ALT\n");
        return NULL;
    }

    alt->is_resolved = 0;
    alt->pri_count = 0;
    alt->guards.num = 0;
    TAILQ_INIT(&alt->guards.Q);

    return alt;
}

void alt_free(Alt *alt)
{
    if (alt == NULL) return;

    Guard *guard;
    TAILQ_FOREACH(guard, &alt->guards.Q, node) {
        alt_guardfree(guard);
    }
    free(alt);
}


void alt_addguard(Alt *alt, Guard *guard)
{
    ASSERT_NOTNULL(alt);

    /* always increment pri_count, to match the switch cases */
    int old_count = alt->pri_count++;
    /* a NULL guard means disabled */
    if (guard == NULL) return;
    
    guard->ch_end->alt = alt;
    guard->pri_case = old_count;
    alt->guards.num++;
    TAILQ_INSERT_TAIL(&alt->guards.Q, guard, node);
}

int alt_select(Alt *alt)
{
    ASSERT_NOTNULL(alt);

    PDEBUG("alt_select finding case\n");

    Guard *guard;
    TAILQ_FOREACH(guard, &alt->guards.Q, node) {
        /* check non-blocking if any guards are ready */
        /* if yes, then complete operation, and return key */
        if (chan_tryread(guard->ch_end, guard->data.ptr, guard->data.size)) {
            PDEBUG("chan was ready\n");
            return guard->pri_case;
        }
        chan_altread(guard->ch_end, guard->data.ptr, guard->data.size);
    }
    
    /* wait until on of the chan_ends reschedules ALT */
    Scheduler *sched = scheduler_self();
    Proc *proc = sched->curr_proc;
    proc->state = PROC_ALTWAIT;
    proc_yield(proc);

    /* remove ALT from rest of guards */ 
    TAILQ_FOREACH(guard, &alt->guards.Q, node) {
        Chan *chan = guard->ch_end->chan;
        if (   chan->state == CHAN_ALTREAD 
            && guard->ch_end->alt == alt) {
            chan->state = CHAN_WAIT;
            chan->data = NULL;
            chan->data_size = 0;
            chan->end_wait = NULL;
        }
    }

    /* FIXME correct case */
    return 0;
}
