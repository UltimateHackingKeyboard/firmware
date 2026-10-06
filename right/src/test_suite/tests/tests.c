#include "tests.h"
#include <stddef.h>

// All test modules registered here
const test_module_t * const AllTestModules[] = {
    &TestModule_Basic,
    &TestModule_Modifiers,
    &TestModule_Layers,
    &TestModule_Macros,
    &TestModule_SecondaryRoles,
    &TestModule_Oneshot,
    &TestModule_Autorepeat,
    &TestModule_Chording,
    &TestModule_AutoShift,
    &TestModule_IfShortcutGesture,
    &TestModule_Doubletap,
    &TestModule_CurrentMacroKeyIsActive,
    &TestModule_ParserBenevolence,
    &TestModule_Sticky,
    &TestModule_Playtime,
    &TestModule_Transport,
    &TestModule_TapKeySeq,
    // Always fails; uncomment to check that the framework reports failures.
    // &TestModule_Fail,
    &TestModule_Link,
};

const uint16_t AllTestModulesCount = sizeof(AllTestModules) / sizeof(AllTestModules[0]);

const test_t* Tests_FindLinkTest(uint8_t linkTestId) {
    if (linkTestId == LinkTestId_None) {
        return NULL;
    }
    for (uint16_t moduleIndex = 0; moduleIndex < AllTestModulesCount; moduleIndex++) {
        const test_module_t *module = AllTestModules[moduleIndex];
        for (uint16_t testIndex = 0; testIndex < module->testCount; testIndex++) {
            if (module->tests[testIndex].linkTestId == linkTestId) {
                return &module->tests[testIndex];
            }
        }
    }
    return NULL;
}
