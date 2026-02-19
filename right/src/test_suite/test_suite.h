#ifndef __TEST_SUITE_H__
#define __TEST_SUITE_H__

#include <stdint.h>
#include <stdbool.h>
#include "str_utils.h"

// Verbose logging mode - when true, logs each action
extern bool TestSuite_Verbose;

// Initialize the test suite. Call once at startup.
void TestSuite_Init(void);

// Run tests. module/test with NULL start = not specified.
// - both NULL: run all
// - module set, test NULL: run all tests in module
// - both set: run single test
uint8_t TestSuite_Run(string_segment_t module, string_segment_t test);

// Run all tests. Returns number of failures.
uint8_t TestSuite_RunAll(void);

// Run a single test by module and test name. Always runs with verbose logging.
// Returns 0 on success, 1 on failure, 255 if test not found.
uint8_t TestSuite_RunSingle(const char *moduleStart, const char *moduleEnd, const char *testStart, const char *testEnd);

#endif
