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
        ModuleFlashState_TryingToReach = 5,
    } module_flash_state_t;

    typedef enum {
        ModuleFlashError_None                 = 0,
        ModuleFlashError_NoFirmware           = 1,
        ModuleFlashError_PingTimeout          = 2,
        ModuleFlashError_PingResponseTimeout  = 3,
        ModuleFlashError_CommandTxFailed      = 4,
        ModuleFlashError_AckTimeout           = 5,
        ModuleFlashError_ResponseTimeout      = 6,
        ModuleFlashError_CommandFailed        = 7,
        ModuleFlashError_DataTxFailed         = 8,
        ModuleFlashError_WriteFailed          = 9,
        ModuleFlashError_FinalResponseTimeout = 10,
    } module_flash_error_t;

// Variables:

    extern module_flash_state_t ModuleFlashState;
    extern bool ModuleFlashBusy;
    extern uint8_t ModuleFlashErrorCode;
    extern uint16_t ModuleFirmwareValidatedSize;

#endif
