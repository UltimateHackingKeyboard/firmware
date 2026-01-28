#ifndef __TEST_SUITE_H__
#define __TEST_SUITE_H__

#include <stdint.h>
#include <stdbool.h>

// Verbose logging mode - when true, logs each action
extern bool TestSuite_Verbose;

// Initialize the test suite. Call once at startup.
void TestSuite_Init(void);

// Run all tests. Returns number of failures.
uint8_t TestSuite_RunAll(void);

// Run a single test by module and test name. Always runs with verbose logging.
// Returns 0 on success, 1 on failure, 255 if test not found.
uint8_t TestSuite_RunSingle(const char *moduleStart, const char *moduleEnd, const char *testStart, const char *testEnd);

#endif
