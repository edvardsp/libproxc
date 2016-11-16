
#include <stdlib.h>

#include "internal.h"

Guard* alt_guardcreate(enum GuardType type, uint64_t usec, 
                       Chan *chan, void *data, size_t size)
{
    /* calloc GUARD struct */
    Guard *guard;
    if (!(guard = calloc(1, sizeof(Guard)))) {
        PERROR("calloc failed for GUARD\n");
        return NULL;
    }

    /* set common members */
    guard->type = type;
    guard->key  = -1;

    /* set type members */
    switch (type) {
    case GUARD_SKIP:
        break;
    case GUARD_TIME:
        guard->usec = usec;
        break;
    case GUARD_CHAN:
        ASSERT_NOTNULL(chan);
        guard->chan = chan;

        guard->ch_end.type  = CHAN_ALTER;
        guard->ch_end.data  = data;
        guard->ch_end.chan  = chan;
        guard->ch_end.guard = guard;

        guard->data.ptr  = data;
        guard->data.size = size;

        guard->in_chan = 0;
        break;
    }

    return guard;
}

void alt_guardfree(Guard *guard)
{
    if (!guard) return;

    free(guard);
}

void alt_init(Alt *alt)
{
    /* alt is not allocated here, as it allows ALT to be */
    /* allocated on either stack or heap */
    ASSERT_NOTNULL(alt);

    alt->key_count   = 0;
    alt->is_accepted = 0;
    alt->winner      = NULL;

    alt->ready.num    = 0;
    alt->ready.guards = NULL;

    alt->guards.num  = 0;
    TAILQ_INIT(&alt->guards.Q);

    alt->guard_skip = NULL;
    alt->guard_time = NULL;
    alt->proc = proc_self();
}

void alt_cleanup(Alt *alt)
{
    if (!alt) return;

    Guard *guard;
    TAILQ_FOREACH(guard, &alt->guards.Q, node) {
        alt_guardfree(guard);
    }
    /* NB! alt is not freed, because ALT is either */
    /* allocated on stack or heap */ 
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
   
    /* Only keep TimeGuard with lowest usce */
    if (guard->type == GUARD_TIME) {
       if (alt->guard_time && (guard->usec >= alt->guard_time->usec)) {
           PDEBUG("AltGuard %d inactive\n", key);
           return;
        }
        alt_guardfree(alt->guard_time);
        alt->guard_time = guard;
    } 

    PDEBUG("AltGuard %d active\n", key);
    guard->key = key;
    guard->alt = alt; 
    guard->ch_end.proc = alt->proc;

    ++alt->guards.num;
    TAILQ_INSERT_TAIL(&alt->guards.Q, guard, node);
}

int alt_accept(Guard *guard)
{
    ASSERT_NOTNULL(guard);

    Alt *alt = guard->alt;
    /* FIXME atomic compare and swap */
    if (alt->is_accepted == 0) {
        PDEBUG("alt_accept succeded!\n");
        alt->is_accepted = 1;
        alt->winner = guard;
        return 1;
    }
    PDEBUG("alt_accept failed\n");
    return 0;
}

int alt_enable(Guard *guard)
{
    ASSERT_NOTNULL(guard);
    
    switch (guard->type) {
    case GUARD_SKIP: return 1;
    case GUARD_TIME: 
        scheduler_addaltsleep(guard);
        return 0;
    case GUARD_CHAN: 
        if (chan_altenable(guard->chan, guard)) {
            return 1;
        }
        guard->in_chan = 1;
        return 0;
    }
    return 0;
}

void alt_disable(Guard *guard)
{
    ASSERT_NOTNULL(guard);
    
    switch (guard->type) {
    case GUARD_SKIP:
        return;
    case GUARD_TIME:
        scheduler_remaltsleep(guard);
        return;
    case GUARD_CHAN:
        if (guard->in_chan) {
            chan_altdisable(guard->chan, guard);
        }
        return;
    }
}

void alt_complete(Alt *alt, Guard *guard)
{
    ASSERT_NOTNULL(alt);
    ASSERT_NOTNULL(guard);

    if (guard->type == GUARD_CHAN && !guard->in_chan) {
        chan_altread(guard->chan, guard, guard->data.size);
    }

    if (alt->ready.num > 0) {
        proc_yield(alt->proc);
    }
}

void alt_choose(Alt *alt)
{
    ASSERT_NOTNULL(alt);

    if (alt->ready.num > 0) {
        /* for now, choose randomly for N > 1 */
        alt->winner = (alt->ready.num > 1)
                    ? alt->ready.guards[rand() % alt->ready.num]
                    : alt->ready.guards[0];
        PDEBUG("One or more ready Guard, key %d wins\n", alt->winner->key);
    }
    /* wait until on of the chan_ends reschedules ALT */
    else if (!alt->is_accepted) {
        PDEBUG("No ready Guards, yield\n");
        alt->proc->state = PROC_ALTWAIT;
        proc_yield(alt->proc);
    }
}

int alt_select(Alt *alt)
{
    ASSERT_NOTNULL(alt);

    PDEBUG("alt_select finding case\n");

    alt->ready.num    = 0;
    alt->ready.guards = malloc(sizeof(Guard *) * alt->guards.num);
    if (UNLIKELY(!alt->ready.guards)) {
        PANIC("Allocation failed for ALT\n");
    }
    
    Guard *guard;
    TAILQ_FOREACH(guard, &alt->guards.Q, node) {
        if (alt_enable(guard)) {
            PDEBUG("AltGuard %d ready\n", guard->key);
            alt->ready.guards[alt->ready.num++] = guard;
        }
    }

    /* determine which Guard is winner, set in alt->winner */
    alt_choose(alt);
    
    /* from here, a winner guard is set in alt->winner */
    ASSERT_NOTNULL(alt->winner);

    TAILQ_FOREACH_REVERSE(guard, &alt->guards.Q, GuardQ, node) {
        alt_disable(guard);
    }

    alt_complete(alt, alt->winner); 

    /* from here, winner contains the winning GUARD */
    return alt->winner->key;
}
