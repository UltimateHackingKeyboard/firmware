#ifndef __MODULE_FLASH_H__
#define __MODULE_FLASH_H__

// Includes:

    #include <stdbool.h>
    #include <stdint.h>

// Typedefs:

    typedef enum {
        ModuleFlashState_Idle    = 0,
        ModuleFlashState_Erasing = 1,
        ModuleFlashState_Writing = 2,
        ModuleFlashState_Done    = 3,
        ModuleFlashState_Error   = 4,
    } module_flash_state_t;

// Variables:

    extern module_flash_state_t ModuleFlashState;
    extern bool ModuleFlashBusy;
    extern uint8_t ModuleFlashErrorCode;

#endif
