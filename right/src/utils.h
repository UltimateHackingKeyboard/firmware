#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

// Includes:

#include "key_states.h"
#include <stdint.h>

// Functions:

    void Utils_SafeStrCopy(char* target, const char* src, uint8_t max);
    key_state_t* Utils_KeyIdToKeyState(uint16_t keyid);
    uint16_t Utils_KeyStateToKeyId(key_state_t* key);
    void Utils_DecodeId(uint16_t keyid, uint8_t* outSlotId, uint8_t* outSlotIdx);


#endif /* SRC_UTILS_H_ */
