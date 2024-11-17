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

    uint8_t UsbCommand_ApplyConfig(void);
    void UsbCommand_ApplyFactory(void);
    void UsbCommand_ApplyConfigAsync(void);

#endif
