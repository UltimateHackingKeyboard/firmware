#include "tests.h"

// All test modules registered here
const test_module_t * const AllTestModules[] = {
    &TestModule_Basic,
    &TestModule_Modifiers,
    &TestModule_Layers,
    &TestModule_Macros,
};

const uint16_t AllTestModulesCount = sizeof(AllTestModules) / sizeof(AllTestModules[0]);
