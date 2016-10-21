
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

    chan->state = CHAN_NOPROCS;
    chan->num_procs = 0;
    TAILQ_INIT(&chan->chanQ);

    *new_chan = chan;
    return 0;
}

void chan_free(Chan *chan)
{
    if (chan == NULL) return;

    free(chan);
}

