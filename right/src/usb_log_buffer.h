#ifndef __USB_LOG_BUFFER_H__
#define __USB_LOG_BUFFER_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>

// Macros:

#define USB_LOG_BUFFER_SIZE 2048

// Variables:

    extern bool UsbLogBuffer_HasLog;

// Functions:

    void UsbLogBuffer_Print(uint8_t *data, uint16_t length);
    uint16_t UsbLogBuffer_Consume(uint8_t* outBuf, uint16_t outBufSize);
    void UsbLogBuffer_GetFill(uint16_t* occupied, uint16_t* size);
    void UsbLogBuffer_SnapToStatusBuffer(void);

#endif
