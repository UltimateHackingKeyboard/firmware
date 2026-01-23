#ifndef __TEST_SUITE_H__
#define __TEST_SUITE_H__

#include <stdint.h>

// Initialize the test suite. Call once at startup.
void TestSuite_Init(void);

// Run all tests. Returns number of failures.
uint8_t TestSuite_RunAll(void);

#endif
