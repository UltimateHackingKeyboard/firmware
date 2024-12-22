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

#ifndef UTILS_ARRAY_SIZE
#define UTILS_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

// Includes:

#include "key_states.h"
#include <stdint.h>

// Typedefs:

    typedef struct {
        uint8_t slotId;
        uint8_t inSlotId;
    } key_coordinates_t;

// Functions:

    void Utils_SafeStrCopy(char* target, const char* src, uint8_t max);
    key_state_t* Utils_KeyIdToKeyState(uint16_t keyid);
    uint16_t Utils_KeyStateToKeyId(key_state_t* key);
    const char* Utils_KeyStateToKeyAbbreviation(key_state_t* key);
    void Utils_DecodeId(uint16_t keyid, uint8_t* outSlotId, uint8_t* outSlotIdx);
    void Utils_PrintReport(const char* prefix, usb_basic_keyboard_report_t* report);
    key_coordinates_t Utils_KeyIdToKeyCoordinates(uint16_t keyId);
    const char* Utils_KeyAbbreviation(key_state_t* keyState);


#endif /* SRC_UTILS_H_ */
