#ifndef __TEST_ACTIONS_H__
#define __TEST_ACTIONS_H__

#include <stdint.h>
#include <stdbool.h>

// Helper macros for defining tests

#define TEST_PRESS(key_id) \
    { .type = TestAction_Press, .keyId = (key_id) }

#define TEST_RELEASE(key_id) \
    { .type = TestAction_Release, .keyId = (key_id) }

#define TEST_DELAY(ms) \
    { .type = TestAction_Delay, .delayMs = (ms) }

// Expect: OutputMachine validates on next report change
// Takes space-separated shortcuts, e.g., TEST_EXPECT("a"), TEST_EXPECT("CS-b a"), TEST_EXPECT("")
#define TEST_EXPECT(shortcuts) \
    { .type = TestAction_Expect, .expectShortcuts = (shortcuts) }

// CheckNow: validate current report immediately (InputMachine)
#define TEST_CHECK_NOW(shortcuts) \
    { .type = TestAction_CheckNow, .expectShortcuts = (shortcuts) }

// SetAction: assign a key action using macro shortcut syntax
#define TEST_SET_ACTION(key_id, shortcut) \
    { .type = TestAction_SetAction, .keyId = (key_id), .shortcutStr = (shortcut) }

#define TEST_END() \
    { .type = TestAction_End }

// Action types

typedef enum {
    TestAction_End,
    TestAction_Press,
    TestAction_Release,
    TestAction_Delay,
    TestAction_SetAction,
    TestAction_Expect,    // OutputMachine only
    TestAction_CheckNow,  // Both machines
} test_action_type_t;

typedef struct {
    test_action_type_t type;
    const char *keyId;
    union {
        uint16_t delayMs;
        const char *expectShortcuts;  // Space-separated shortcut strings
        const char *shortcutStr;      // For SetAction
    };
} test_action_t;

// Test definition

typedef struct {
    const char *name;
    const test_action_t *actions;
} test_t;

#endif
