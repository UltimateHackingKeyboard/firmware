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
extern const test_module_t TestModule_Doubletap;
extern const test_module_t TestModule_CurrentMacroKeyIsActive;
extern const test_module_t TestModule_ParserBenevolence;
extern const test_module_t TestModule_Sticky;
extern const test_module_t TestModule_Playtime;
extern const test_module_t TestModule_Transport;
extern const test_module_t TestModule_TapKeySeq;
extern const test_module_t TestModule_Fail;
extern const test_module_t TestModule_Link;

// Link test ids are the wire contract between the halves: the right half sends one in
// MessengerCommand_StartTest and the left looks the test up by it. Keep them stable.
typedef enum {
    LinkTestId_None = 0,
    LinkTestId_Probe = 3,
} link_test_id_t;

// The test carrying this link test id, in any module, or NULL for LinkTestId_None or an
// unknown id. Both halves resolve an id to the same test, which is how they stay on one script.
const test_t* Tests_FindLinkTest(uint8_t linkTestId);

#endif
