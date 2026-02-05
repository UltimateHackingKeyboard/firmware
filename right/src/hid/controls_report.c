#include "hid/controls_report.h"
#include <string.h>
#include "utils.h"

void ControlsReport_MergeReports(
    const hid_controls_report_t *sourceReport, hid_controls_report_t *targetReport)
{
    uint8_t idx, i = 0;
    /* find empty position */
    for (idx = 0; idx < UTILS_ARRAY_SIZE(targetReport->consumer); idx++) {
        if (targetReport->consumer[idx] == 0) {
            break;
        }
    }
    /* copy into empty positions */
    while ((i < UTILS_ARRAY_SIZE(sourceReport->consumer)) && (sourceReport->consumer[i] != 0) &&
           (idx < UTILS_ARRAY_SIZE(targetReport->consumer))) {
        targetReport->consumer[idx++] = sourceReport->consumer[i++];
    }

    for (idx = 0; idx < UTILS_ARRAY_SIZE(targetReport->system); idx++) {
        if (targetReport->system[idx] == 0) {
            break;
        }
    }
    while ((i < UTILS_ARRAY_SIZE(sourceReport->system)) && (sourceReport->system[i] != 0) &&
           (idx < UTILS_ARRAY_SIZE(targetReport->system))) {
        targetReport->system[idx++] = sourceReport->system[i++];
    }
}

bool ControlsReport_AddConsumerUsage(hid_controls_report_t *report, uint16_t scancode)
{
    if (scancode == 0) {
        return true;
    }
    for (uint8_t i = 0; i < UTILS_ARRAY_SIZE(report->consumer); i++) {
        if (report->consumer[i] == 0) {
            report->consumer[i] = scancode;
            return true;
        }
    }
    return false;
}

bool ControlsReport_RemoveConsumerUsage(hid_controls_report_t *report, uint16_t scancode)
{
    if (scancode == 0) {
        return false;
    }
    for (uint8_t i = 0; i < UTILS_ARRAY_SIZE(report->consumer); i++) {
        if (report->consumer[i] == scancode) {
            report->consumer[i] = 0;
            return true;
        }
    }
    return false;
}

bool ControlsReport_AddSystemUsage(hid_controls_report_t *report, uint8_t scancode)
{
    if (scancode == 0) {
        return true;
    }
    for (uint8_t i = 0; i < UTILS_ARRAY_SIZE(report->system); i++) {
        if (report->system[i] == 0) {
            report->system[i] = scancode;
            return true;
        }
    }
    return false;
}

bool ControlsReport_RemoveSystemUsage(hid_controls_report_t *report, uint8_t scancode)
{
    if (scancode == 0) {
        return true;
    }
    for (uint8_t i = 0; i < UTILS_ARRAY_SIZE(report->system); i++) {
        if (report->system[i] == scancode) {
            report->system[i] = 0;
            return true;
        }
    }
    return false;
}

bool ControlsReport_ContainsConsumerUsage(const hid_controls_report_t *report, uint16_t scancode)
{
    for (uint8_t i = 0; i < UTILS_ARRAY_SIZE(report->consumer); i++) {
        if (report->consumer[i] == scancode) {
            return true;
        }
    }
    return false;
}

bool ControlsReport_ContainsSystemUsage(const hid_controls_report_t *report, uint8_t scancode)
{
    for (uint8_t i = 0; i < UTILS_ARRAY_SIZE(report->system); i++) {
        if (report->system[i] == scancode) {
            return true;
        }
    }
    return false;
}

bool ControlsReport_HasChanges(const hid_controls_report_t buffer[2])
{
    return memcmp(&buffer[0], &buffer[1], sizeof(hid_controls_report_t)) != 0;
}
