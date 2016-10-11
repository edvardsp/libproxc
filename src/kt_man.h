
#ifndef KT_MAN_H__
#define KT_MAN_H__

#include <pthread.h>

void ktm_init(pthread_t main_thread);
void ktm_cleanup(void);

#endif /* KT_MAN_H__ */

