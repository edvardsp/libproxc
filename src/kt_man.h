
#ifndef KT_MAN_H__
#define KT_MAN_H__

#include <ucontext.h>

void ktm_init(ucontext_t main_ctx);
void ktm_cleanup(void);

#endif /* KT_MAN_H__ */

