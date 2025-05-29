#ifndef __USB_COMMAND_APPLY_CONFIG_H__
#define __USB_COMMAND_APPLY_CONFIG_H__

// Includes:

    #include <stdbool.h>
    #include <stdint.h>

// Typedefs:

    typedef enum {
        ParsingStage_Validate,
        ParsingStage_Apply,
    } parser_stage_t;

// Functions:

    void UsbCommand_ApplyFactory(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);
    uint8_t UsbCommand_ApplyConfigSync(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);
    uint8_t UsbCommand_ValidateAndApplyConfigSync(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);
    uint8_t UsbCommand_ValidateAndApplyConfigAsync(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);

#endif
