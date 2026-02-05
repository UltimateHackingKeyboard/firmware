#ifndef __HID_KEYBOARD_REPORT_H__
#define __HID_KEYBOARD_REPORT_H__

#include "attributes.h"
#include "lufa/HIDClassCommon.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define HID_KEYBOARD_MIN_MODIFIERS_SCANCODE (HID_KEYBOARD_SC_LEFT_CONTROL)
#define HID_KEYBOARD_MAX_MODIFIERS_SCANCODE (HID_KEYBOARD_SC_RIGHT_GUI)
#if (HID_KEYBOARD_MAX_MODIFIERS_SCANCODE - HID_KEYBOARD_MIN_MODIFIERS_SCANCODE + 1) != 8
    #error HID_KEYBOARD modifiers range invalid, adjust report layout and report descriptor
#endif

// the first 4 codes are error codes, so not needed in bitmask
#define HID_KEYBOARD_MIN_BITFIELD_SCANCODE (HID_KEYBOARD_SC_A)
// this bitfield goes up all the way until the modifiers
#define HID_KEYBOARD_MAX_BITFIELD_SCANCODE (CONFIG_KEYBOARD_MAX_SCANCODE)

#define HID_KEYBOARD_BITFIELD_COUNT                                                                \
    (HID_KEYBOARD_MAX_BITFIELD_SCANCODE - HID_KEYBOARD_MIN_BITFIELD_SCANCODE + 1)
#define HID_KEYBOARD_BITFIELD_LENGTH ((HID_KEYBOARD_BITFIELD_COUNT + 7) / 8)

#if HID_KEYBOARD_MAX_BITFIELD_SCANCODE < 0x65
    #warning HID_KEYBOARD_MAX_BITFIELD_SCANCODE less than maximum standard keyboard scancodes
#endif

typedef struct {
    uint8_t modifiers;
    union {
        struct {
            uint8_t reserved;
            uint8_t scancodes[6];
        } ATTR_PACKED boot;
        uint8_t bitfield[HID_KEYBOARD_BITFIELD_LENGTH];
    } ATTR_PACKED;
} ATTR_PACKED hid_keyboard_report_t;

typedef hid_keyboard_report_t usb_basic_keyboard_report_t;

typedef struct {
    bool capsLock;
    bool numLock;
    bool scrollLock;
} ATTR_PACKED hid_keyboard_led_state_t;

extern hid_keyboard_led_state_t KeyboardLedsState;
#define UsbBasicKeyboard_CapsLockOn (KeyboardLedsState.capsLock)
#define UsbBasicKeyboard_NumLockOn (KeyboardLedsState.numLock)
#define UsbBasicKeyboard_ScrollLockOn (KeyboardLedsState.scrollLock)

static inline bool KeyboardReport_IsModifier(uint8_t scancode)
{
    return (scancode >= HID_KEYBOARD_MIN_MODIFIERS_SCANCODE) &&
            (scancode <= HID_KEYBOARD_MAX_MODIFIERS_SCANCODE);
}
bool KeyboardReport_AddScancode(hid_keyboard_report_t *report, uint8_t scancode);
#define UsbBasicKeyboard_AddScancode KeyboardReport_AddScancode
void KeyboardReport_RemoveScancode(hid_keyboard_report_t *report, uint8_t scancode);
#define UsbBasicKeyboard_RemoveScancode KeyboardReport_RemoveScancode
bool KeyboardReport_ContainsScancode(const hid_keyboard_report_t *report, uint8_t scancode);
#define UsbBasicKeyboard_ContainsScancode KeyboardReport_ContainsScancode
size_t KeyboardReport_ScancodeCount(const hid_keyboard_report_t *report);
#define UsbBasicKeyboard_ScancodeCount KeyboardReport_ScancodeCount
void KeyboardReport_MergeReports(
    const hid_keyboard_report_t *sourceReport, hid_keyboard_report_t *targetReport);
#define UsbBasicKeyboard_MergeReports KeyboardReport_MergeReports
void KeyboardReport_ForeachScancode(const hid_keyboard_report_t *report, void (*action)(uint8_t));
#define UsbBasicKeyboard_ForeachScancode KeyboardReport_ForeachScancode
bool KeyboardReport_FindFirstDifference(
    const hid_keyboard_report_t *current, const hid_keyboard_report_t *previous, uint8_t *result);
#define UsbBasicKeyboard_FindFirstDifference KeyboardReport_FindFirstDifference

bool KeyboardReport_HasChange(const hid_keyboard_report_t buffers[2]);

#endif // __HID_KEYBOARD_REPORT_H__
