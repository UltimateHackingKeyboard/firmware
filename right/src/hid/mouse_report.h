#ifndef __HID_MOUSE_REPORT_H__
#define __HID_MOUSE_REPORT_H__

#include "attributes.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define HID_MOUSE_BUTTON_COUNT 24

typedef struct {
    uint32_t buttons : HID_MOUSE_BUTTON_COUNT;
    int16_t x;
    int16_t y;
    int16_t wheelY;
    int16_t wheelX;
} ATTR_PACKED hid_mouse_report_t;

float VerticalScrollMultiplier(void);
float HorizontalScrollMultiplier(void);

void MouseReport_MergeReports(hid_mouse_report_t *sourceReport, hid_mouse_report_t *targetReport);
bool MouseReport_HasChanges(const hid_mouse_report_t buffer[2], const hid_mouse_report_t *active);

#endif // __HID_MOUSE_REPORT_H__
