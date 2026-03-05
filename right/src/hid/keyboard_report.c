#include "hid/keyboard_report.h"
#include "utils.h"
#include <string.h>

hid_keyboard_led_state_t KeyboardLedsState;

static inline bool KeyboardReport_IsInBitfield(uint8_t scancode)
{
    return (scancode >= HID_KEYBOARD_MIN_BITFIELD_SCANCODE) &&
           (scancode <= HID_KEYBOARD_MAX_BITFIELD_SCANCODE);
}

bool KeyboardReport_AddScancode(hid_keyboard_report_t *report, uint8_t scancode)
{
    if (scancode == 0)
        return true;

    if (KeyboardReport_IsModifier(scancode)) {
        // modifiers are kept the same place in both report layouts
        set_bit(scancode - HID_KEYBOARD_MIN_MODIFIERS_SCANCODE, &report->modifiers);
        return true;
    } else if (KeyboardReport_IsInBitfield(scancode)) {
        set_bit(scancode - HID_KEYBOARD_MIN_BITFIELD_SCANCODE, report->bitfield);
        return true;
    } else {
        return false;
    }
}

void KeyboardReport_RemoveScancode(hid_keyboard_report_t *report, uint8_t scancode)
{
    if (KeyboardReport_IsModifier(scancode)) {
        // modifiers are kept the same place in both report layouts
        clear_bit(scancode - HID_KEYBOARD_MIN_MODIFIERS_SCANCODE, &report->modifiers);
    } else if (KeyboardReport_IsInBitfield(scancode)) {
        clear_bit(scancode - HID_KEYBOARD_MIN_BITFIELD_SCANCODE, report->bitfield);
    }
}

bool KeyboardReport_ContainsScancode(const hid_keyboard_report_t *report, uint8_t scancode)
{
    if (KeyboardReport_IsModifier(scancode)) {
        // modifiers are kept the same place in both report layouts
        return test_bit(scancode - HID_KEYBOARD_MIN_MODIFIERS_SCANCODE, &report->modifiers);
    } else if (KeyboardReport_IsInBitfield(scancode)) {
        return test_bit(scancode - HID_KEYBOARD_MIN_BITFIELD_SCANCODE, report->bitfield);
    } else {
        return false;
    }
}

size_t KeyboardReport_ScancodeCount(const hid_keyboard_report_t *report)
{
    size_t size = 0;
    for (uint8_t i = 0; i < UTILS_ARRAY_SIZE(report->bitfield); i++) {
        size += __builtin_popcount(report->bitfield[i]);
    }
    return size;
}

void KeyboardReport_MergeReports(
    const hid_keyboard_report_t *sourceReport, hid_keyboard_report_t *targetReport)
{
    targetReport->modifiers |= sourceReport->modifiers;

    for (uint8_t i = 0; i < UTILS_ARRAY_SIZE(targetReport->bitfield); i++) {
        targetReport->bitfield[i] |= sourceReport->bitfield[i];
    }
}

void KeyboardReport_ForeachScancode(const hid_keyboard_report_t *report, void (*action)(uint8_t))
{
    // ignoring modifiers
    for (uint8_t i = 0; i < UTILS_ARRAY_SIZE(report->bitfield); i++) {
        for (uint8_t j = 0, b = report->bitfield[i]; b > 0; j++, b >>= 1) {
            if (b & 1) {
                action(HID_KEYBOARD_MIN_BITFIELD_SCANCODE + i * 8 + j);
            }
        }
    }
}

bool KeyboardReport_FindFirstDifference(
    const hid_keyboard_report_t *current, const hid_keyboard_report_t *previous, uint8_t *result)
{
    // Check modifier differences first
    if (current->modifiers != previous->modifiers) {
        // Check left modifiers (bits 0-3)
        uint8_t leftModsCurrent = current->modifiers & 0x0F;
        uint8_t leftModsPrevious = previous->modifiers & 0x0F;
        if (leftModsCurrent != leftModsPrevious) {
            *result = 0xC0 | leftModsCurrent; // 1100 + 4 bits
            return true;
        }

        // Check right modifiers (bits 4-7)
        uint8_t rightModsCurrent = (current->modifiers & 0xF0) >> 4;
        uint8_t rightModsPrevious = (previous->modifiers & 0xF0) >> 4;
        if (rightModsCurrent != rightModsPrevious) {
            *result = 0xE0 | rightModsCurrent; // 1110 + 4 bits
            return true;
        }
    }

    // Check scancode differences (bitfield only)
    for (uint8_t i = 0; i < UTILS_ARRAY_SIZE(current->bitfield); i++) {
        if (current->bitfield[i] != previous->bitfield[i]) {
            // Find the first differing bit
            uint8_t diff = current->bitfield[i] ^ previous->bitfield[i];
            for (uint8_t j = 0; j < 8; j++) {
                if (diff & (1 << j)) {
                    uint8_t scancode = HID_KEYBOARD_MIN_BITFIELD_SCANCODE + i * 8 + j;
                    if (scancode > 127) {
                        // Scancode out of range for 7-bit encoding, signal to use full report
                        *result = 0xFF; // Special value indicating out of range
                        return true;
                    }
                    *result = scancode & 0x7F; // 0 + 7 bits of scancode
                    return true;
                }
            }
        }
    }

    return false; // No differences found
}

bool KeyboardReport_HasChange(const hid_keyboard_report_t buffers[2])
{
    return memcmp(&buffers[0], &buffers[1], sizeof(hid_keyboard_report_t)) != 0;
}
