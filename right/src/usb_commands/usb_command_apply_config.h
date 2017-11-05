#ifndef __USB_COMMAND_APPLY_CONFIG_H__
#define __USB_COMMAND_APPLY_CONFIG_H__

// Typedefs:

typedef enum {
    ParsingStage_Validate,
    ParsingStage_Apply,
} parsing_stage_t;

// Functions:

    void UsbCommand_ApplyConfig(void);

#endif
