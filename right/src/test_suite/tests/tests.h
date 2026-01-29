#ifndef __TEST_SUITE_TESTS_H__
#define __TEST_SUITE_TESTS_H__

#include "test_suite/test_actions.h"
#include <stdint.h>

// Test module: a collection of related tests
typedef struct {
    const char *name;
    const test_t *tests;
    uint16_t testCount;
} test_module_t;

// All test modules (defined in tests.c)
extern const test_module_t * const AllTestModules[];
extern const uint16_t AllTestModulesCount;

// Individual test modules (defined in separate files)
extern const test_module_t TestModule_Basic;
extern const test_module_t TestModule_Modifiers;
extern const test_module_t TestModule_Layers;
extern const test_module_t TestModule_Macros;
extern const test_module_t TestModule_SecondaryRoles;
extern const test_module_t TestModule_Oneshot;
extern const test_module_t TestModule_Autorepeat;
extern const test_module_t TestModule_Chording;
extern const test_module_t TestModule_AutoShift;
extern const test_module_t TestModule_IfShortcutGesture;

#endif
