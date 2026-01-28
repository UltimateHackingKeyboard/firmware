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

// ExpectMaybe: if report matches, consume; if not, skip without failing
#define TEST_EXPECT_MAYBE(shortcuts) \
    { .type = TestAction_ExpectMaybe, .expectShortcuts = (shortcuts) }

// CheckNow: validate current report immediately (InputMachine)
#define TEST_CHECK_NOW(shortcuts) \
    { .type = TestAction_CheckNow, .expectShortcuts = (shortcuts) }

// SetAction: assign a key action using macro shortcut syntax
#define TEST_SET_ACTION(key_id, shortcut) \
    { .type = TestAction_SetAction, .keyId = (key_id), .shortcutStr = (shortcut) }

// SetMacro: assign an inline macro to a key
#define TEST_SET_MACRO(key_id, macro_text) \
    { .type = TestAction_SetMacro, .keyId = (key_id), .macroText = (macro_text) }

// SetLayerHold: assign a layer hold action to a key
#define TEST_SET_LAYER_HOLD(key_id, layer_id) \
    { .type = TestAction_SetLayerHold, .keyId = (key_id), .layerId = (layer_id) }

// SetLayerAction: assign a key action on a specific layer
#define TEST_SET_LAYER_ACTION(layer_id, key_id, shortcut) \
    { .type = TestAction_SetLayerAction, .keyId = (key_id), .layerId = (layer_id), .shortcutStr = (shortcut) }

// SetSecondaryRole: assign a keystroke with secondary role modifier
// secondary_role is one of: SecondaryRole_LeftShift, SecondaryRole_LeftCtrl, etc.
#define TEST_SET_SECONDARY_ROLE(key_id, primary_scancode, secondary_role) \
    { .type = TestAction_SetSecondaryRole, .keyId = (key_id), .primaryScancode = (primary_scancode), .secondaryRoleId = (secondary_role) }

// SetConfig: run a set command to configure settings
#define TEST_SET_CONFIG(config_text) \
    { .type = TestAction_SetConfig, .configText = (config_text) }

#define TEST_END() \
    { .type = TestAction_End }

// Action types

typedef enum {
    TestAction_End,
    TestAction_Press,
    TestAction_Release,
    TestAction_Delay,
    TestAction_SetAction,
    TestAction_SetMacro,
    TestAction_SetLayerHold,
    TestAction_SetLayerAction,
    TestAction_SetSecondaryRole,
    TestAction_SetConfig,
    TestAction_Expect,       // OutputMachine only
    TestAction_ExpectMaybe,  // OutputMachine only, optional
    TestAction_CheckNow,     // Both machines
} test_action_type_t;

typedef struct {
    test_action_type_t type;
    const char *keyId;
    uint8_t layerId;              // For SetLayerHold, SetLayerAction
    uint8_t primaryScancode;      // For SetSecondaryRole
    uint8_t secondaryRoleId;      // For SetSecondaryRole
    union {
        uint16_t delayMs;
        const char *expectShortcuts;  // Space-separated shortcut strings
        const char *shortcutStr;      // For SetAction, SetLayerAction
        const char *macroText;        // For SetMacro
        const char *configText;       // For SetConfig
    };
} test_action_t;

// Test definition

typedef struct {
    const char *name;
    const test_action_t *actions;
} test_t;

#endif
