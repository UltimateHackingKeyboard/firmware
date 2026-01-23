#ifndef __TEST_ACTIONS_H__
#define __TEST_ACTIONS_H__

#include <stdint.h>
#include <stdbool.h>
#include "key_action.h"

// Helper macros for defining tests

#define TEST_PRESS(key_id) \
    { .type = TestAction_Press, .keyId = (key_id) }

#define TEST_RELEASE(key_id) \
    { .type = TestAction_Release, .keyId = (key_id) }

#define TEST_DELAY(ms) \
    { .type = TestAction_Delay, .delayMs = (ms) }

#define TEST_EXPECT(mods, ...) \
    { .type = TestAction_Expect, .expect = { .modifiers = (mods), .scancodes = { __VA_ARGS__ } } }

#define TEST_EXPECT_EMPTY() \
    { .type = TestAction_Expect, .expect = { 0 } }

#define TEST_SET_SCANCODE(key_id, scancode) \
    { .type = TestAction_SetAction, .keyId = (key_id), .action = { \
        .type = KeyActionType_Keystroke, \
        .keystroke = { .keystrokeType = KeystrokeType_Basic, .scancode = (scancode) } \
    } }

#define TEST_END() \
    { .type = TestAction_End }

// Action types

typedef enum {
    TestAction_End,
    TestAction_Press,
    TestAction_Release,
    TestAction_Delay,
    TestAction_Expect,
    TestAction_SetAction,
} test_action_type_t;

typedef struct {
    uint8_t modifiers;
    uint8_t scancodes[6];
} test_expected_report_t;

typedef struct {
    test_action_type_t type;
    const char *keyId;
    union {
        uint16_t delayMs;
        test_expected_report_t expect;
        key_action_t action;
    };
} test_action_t;

// Test definition

typedef struct {
    const char *name;
    const test_action_t *actions;
} test_t;

#endif
