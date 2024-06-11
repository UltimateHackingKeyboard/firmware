#ifndef __USB_HEADER__
#define __USB_HEADER__


// Includes:

    #include <zephyr/device.h>
    #include <inttypes.h>
    #include <stdbool.h>

// Macros:

    #define SEMAPHORE_RESET_TIMEOUT 100

// Typedefs:

    typedef enum
    {
        Rollover_NKey = 0,
        Rollover_6Key = 1,
    } rollover_t;

    typedef enum
    {
        Hid_Empty = 0,
        Hid_NoGamepad,
        Hid_Full,
    } hid_config_t;

// Variables:

// Functions:

    rollover_t HID_GetKeyboardRollover(void);
    void HID_SetKeyboardRollover(rollover_t mode);
    void HID_SetGamepadActive(bool active);
    bool HID_GetGamepadActive(void);
    void HOGP_Enable(void);
    void HOGP_Disable(void);
    void USB_DisableHid(void);
    void USB_EnableHid(void);

#endif // __USB_HEADER__
