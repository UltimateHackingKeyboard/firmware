#ifndef __HID_CONTROLS_REPORT_H__
#define __HID_CONTROLS_REPORT_H__

#include "arduino_hid/SystemAPI.h"
#include "attributes.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define HID_CONTROLS_MAX_CONSUMER_KEYS 4
#define HID_CONTROLS_MAX_SYSTEM_KEYS 2

#define HID_CONTROLS_MIN_SYSTEM_USAGE SYSTEM_POWER_DOWN
#define HID_CONTROLS_MAX_SYSTEM_USAGE SYSTEM_WAKE_UP

typedef struct {
    uint16_t consumer[HID_CONTROLS_MAX_CONSUMER_KEYS];
    uint8_t system[HID_CONTROLS_MAX_SYSTEM_KEYS];
} ATTR_PACKED hid_controls_report_t;

void ControlsReport_MergeReports(
    const hid_controls_report_t *sourceReport, hid_controls_report_t *targetReport);
bool ControlsReport_AddConsumerUsage(hid_controls_report_t *report, uint16_t scancode);
bool ControlsReport_RemoveConsumerUsage(hid_controls_report_t *report, uint16_t scancode);
bool ControlsReport_AddSystemUsage(hid_controls_report_t *report, uint8_t scancode);
bool ControlsReport_RemoveSystemUsage(hid_controls_report_t *report, uint8_t scancode);
bool ControlsReport_ContainsConsumerUsage(
    const hid_controls_report_t *report, uint16_t scancode);
bool ControlsReport_ContainsSystemUsage(
    const hid_controls_report_t *report, uint8_t scancode);
bool ControlsReport_HasChanges(const hid_controls_report_t buffer[2]);

#endif // __HID_CONTROLS_REPORT_H__
