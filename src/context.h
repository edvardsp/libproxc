
#ifndef CONTEXT_H__
#define CONTEXT_H__

#include <ucontext.h>

#include "internal.h"

int context_create(Context **new_context, ucontext_t *ctx);

#endif /* CONTEXT_H__ */

