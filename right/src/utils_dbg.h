// #define DEBUG

#ifdef DEBUG
#ifndef SRC_UTILS_DBG_H_
#define SRC_UTILS_DBG_H_

#include <stdint.h>
#include "key_states.h"

#define WATCH_TRIGGER(STATE) TriggerWatch(STATE);
#define WATCH_TIME(N) if(CurrentWatch == N) { WatchTime(N); }
#define WATCH_VALUE(V, N) if(CurrentWatch == N) { WatchValue(V, N); }

extern uint8_t CurrentWatch;

void TriggerWatch(key_state_t *keyState);
void WatchTime(uint8_t n);
void WatchValue(int v, uint8_t n);
void ShowNumberMag(int a);

#endif /* SRC_UTILS_DBG_H_ */

#else

#define WATCH_TRIGGER(N)
#define WATCH_TIME(N)
#define WATCH_VALUE(V, N)

#endif
