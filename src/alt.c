
#include <stdlib.h>

#include "internal.h"

Guard* alt_guardcreate(Chan *ch, void *data, size_t size)
{
    ASSERT_NOTNULL(ch);

    /* alloc GUARD struct */
    Guard *guard;
    if (!(guard = malloc(sizeof(Guard)))) {
        PERROR("malloc failed for GUARD\n");
        return NULL;
    }

    /* set struct members */
    guard->key       = -1;
    guard->alt       = NULL;
    guard->chan      = ch;
    guard->data.ptr  = data;
    guard->data.size = size;

    struct ChanEnd ch_end = {
        .type = CHAN_ALTER,
        .data = data,
        .chan = ch,
        .proc = NULL,
        .guard = guard
    };
    guard->ch_end = ch_end;

    return guard;
}

void alt_guardfree(Guard *guard)
{
    if (!guard) return;

    free(guard);
}

Alt* alt_create(void)
{
    /* alloc ALT struct */
    Alt *alt;
    if (!(alt = malloc(sizeof(Alt)))) {
        PERROR("malloc failed for ALT\n");
        return NULL;
    }

    alt->key_count   = 0;
    alt->is_accepted = 0;
    alt->key_accept  = -1;
    alt->guards.num  = 0;
    TAILQ_INIT(&alt->guards.Q);
    alt->alt_proc = proc_self();

    return alt;
}

void alt_free(Alt *alt)
{
    if (!alt) return;

    Guard *guard;
    TAILQ_FOREACH(guard, &alt->guards.Q, node) {
        alt_guardfree(guard);
    }
    free(alt);
}

void alt_addguard(Alt *alt, Guard *guard)
{
    ASSERT_NOTNULL(alt);

    /* always increment key_count, to match the switch cases */
    int key = alt->key_count++;
    /* a NULL guard means not active */
    if (!guard) {
        PDEBUG("AltGuard %d inactive\n", key);
        return;
    }
   
    PDEBUG("AltGuard %d active\n", key);
    guard->key = key;
    guard->alt = alt; 
    guard->ch_end.proc = alt->alt_proc;

    alt->guards.num++;
    TAILQ_INSERT_TAIL(&alt->guards.Q, guard, node);
}

int alt_accept(Guard *guard)
{
    ASSERT_NOTNULL(guard);

    Alt *alt = guard->alt;
    /* FIXME atomic compare and swap */
    if (!alt->is_accepted) {
        PDEBUG("alt_accept succeded!\n");
        alt->is_accepted = 1;
        alt->key_accept = guard->key;
        return 1;
    }
    PDEBUG("alt_accept failed\n");
    return 0;
}

int alt_enable(Guard *guard)
{
    ASSERT_NOTNULL(guard);
    
    return chan_altread(guard->chan, guard, guard->data.ptr, guard->data.size);
}

void alt_disable(Guard *guard)
{
    ASSERT_NOTNULL(guard);
    

    Chan *chan = guard->chan;
    if (TAILQ_EMPTY(&chan->altQ)) {
        return;
    }
    ChanEnd *ch_end = &guard->ch_end;
    TAILQ_REMOVE(&chan->altQ, ch_end, node);
}

int alt_select(Alt *alt)
{
    ASSERT_NOTNULL(alt);

    PDEBUG("alt_select finding case\n");

    Guard *guard;
    TAILQ_FOREACH(guard, &alt->guards.Q, node) {
        if (alt_enable(guard)) {
            goto AltAccepted;
        }
    }
    
    /* wait until on of the chan_ends reschedules ALT */
    if (!alt->is_accepted) {
        Proc *proc = scheduler_self()->curr_proc;
        proc->state = PROC_ALTWAIT;
        proc_yield(proc);
    }

AltAccepted: /* Only one guard is accepted from here */

    TAILQ_FOREACH_REVERSE(guard, &alt->guards.Q, GuardQ, node) {
        alt_disable(guard);
    }


    /* from here, winner contains the winning GUARD */
    return alt->key_accept;
}
