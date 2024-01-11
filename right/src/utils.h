#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

#include "usb_interfaces/usb_interface_basic_keyboard.h"

// Macros:

#define REENTRANCY_GUARD_BEGIN                   \
static bool reentrancyGuard_active = false;      \
if (reentrancyGuard_active) {                    \
    return;                                      \
} else {                                         \
    reentrancyGuard_active = true;               \
}
#define REENTRANCY_GUARD_END                     \
    reentrancyGuard_active = false;

// Includes:

#include "key_states.h"
#include <stdint.h>

// Functions:

    void Utils_SafeStrCopy(char* target, const char* src, uint8_t max);
    key_state_t* Utils_KeyIdToKeyState(uint16_t keyid);
    uint16_t Utils_KeyStateToKeyId(key_state_t* key);
    void Utils_DecodeId(uint16_t keyid, uint8_t* outSlotId, uint8_t* outSlotIdx);
    void Utils_PrintReport(const char* prefix, usb_basic_keyboard_report_t* report);


#endif /* SRC_UTILS_H_ */
