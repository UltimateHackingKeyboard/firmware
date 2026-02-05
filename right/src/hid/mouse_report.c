#include "mouse_report.h"
#include <string.h>

void MouseReport_MergeReports(hid_mouse_report_t *sourceReport, hid_mouse_report_t *targetReport)
{
    targetReport->buttons |= sourceReport->buttons;
    targetReport->x += sourceReport->x;
    targetReport->y += sourceReport->y;
    targetReport->wheelX += sourceReport->wheelX;
    targetReport->wheelY += sourceReport->wheelY;

    sourceReport->x = 0;
    sourceReport->y = 0;
    sourceReport->wheelX = 0;
    sourceReport->wheelY = 0;
}

bool MouseReport_HasChanges(const hid_mouse_report_t buffer[2], const hid_mouse_report_t *active)
{
    // Send out the mouse position and wheel values continuously if the report is not zeros, but only send the mouse button states when they change.
    int movement = active->x | active->y | active->wheelX | active->wheelY;
    return (memcmp(&buffer[0], &buffer[1], sizeof(hid_mouse_report_t)) != 0) || (movement != 0);
}
