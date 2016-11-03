
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
    guard->key       = -1;
    guard->alt       = NULL;
    guard->ch_end    = ch_end;
    guard->data.ptr  = out;
    guard->data.size = size;

    return guard;
}

void alt_guardfree(Guard *guard)
{
    if (guard == NULL) return;

    /* reset CHANEND */
    guard->ch_end->guard = NULL;

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

    alt->key_count   = 0;
    alt->is_accepted = 0;
    alt->accepted    = NULL;
    alt->guards.num  = 0;
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

    /* always increment key_count, to match the switch cases */
    int key = alt->key_count++;
    /* a NULL guard means not active */
    if (guard == NULL) {
        PDEBUG("AltGuard %d inactive\n", key);
        return;
    }
   
    PDEBUG("AltGuard %d active\n", key);
    guard->key = key;
    guard->alt = alt; 

    alt->guards.num++;
    TAILQ_INSERT_TAIL(&alt->guards.Q, guard, node);
}

int alt_accept(Alt *alt, Guard *guard)
{
    ASSERT_NOTNULL(alt);
    ASSERT_NOTNULL(guard);

    /* FIXME atomic compare and swap */
    if (!alt->is_accepted) {
        PDEBUG("alt_accept succeded!\n");
        alt->is_accepted = 1;
        alt->accepted = guard;
        return 1;
    }
    PDEBUG("alt_accept failed\n");
    return 0;
}

int alt_enable(Alt *alt, Guard *guard)
{
    ASSERT_NOTNULL(alt);
    ASSERT_NOTNULL(guard);

    ChanEnd *ch_end = guard->ch_end;
    ASSERT_NOTNULL(ch_end);

    ch_end->guard = guard;
    chan_altenable(ch_end, guard->data.ptr, guard->data.size);

    return alt->is_accepted;
}

void alt_disable(Alt *alt, Guard *guard)
{
    ASSERT_NOTNULL(alt);
    ASSERT_NOTNULL(guard);

    chan_altdisable(guard->ch_end);
}

int alt_select(Alt *alt)
{
    ASSERT_NOTNULL(alt);

    PDEBUG("alt_select finding case\n");

    Guard *guard;
    TAILQ_FOREACH(guard, &alt->guards.Q, node) {
        if (alt_enable(alt, guard)) {
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
        alt_disable(alt, guard);
    }


    /* from here, winner contains the winning GUARD */
    ASSERT_NOTNULL(alt->accepted);
    return alt->accepted->key;
}
