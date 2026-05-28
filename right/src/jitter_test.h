#ifndef __JITTER_TEST_H__
#define __JITTER_TEST_H__

#include <stdbool.h>
#include <stdint.h>

extern bool JitterTest_Active;

void JitterTest_SetActive(bool active);
void JitterTest_RecordMouseX(int16_t x);

#endif
