#include "usb_log_buffer.h"
#include "str_utils.h"
#include "wormhole.h"
#include "macros/status_buffer.h"

static char buffer[USB_LOG_BUFFER_SIZE];
static uint16_t bufferPosition = 0;
static uint16_t bufferLength = 0;

bool UsbLogBuffer_HasLog = false;

#define POS(x) ((bufferPosition + (x) + USB_LOG_BUFFER_SIZE) % USB_LOG_BUFFER_SIZE)

static void updateNonemptyFlag() {
    UsbLogBuffer_HasLog = (bufferLength > 0);
}

static void addChar(char c) {
    if (CHAR_IS_VALID(c)) {
        if (bufferLength < USB_LOG_BUFFER_SIZE) {
            buffer[POS(bufferLength)] = c;
            bufferLength++;
        } else {
            buffer[bufferPosition++] = c;
            bufferPosition %= USB_LOG_BUFFER_SIZE;
        }
    }
}

void UsbLogBuffer_Print(uint8_t *data, uint16_t length) {
    for (uint16_t i = 0; i < length; i++) {
        addChar(data[i]);
    }
    updateNonemptyFlag();
}

uint16_t UsbLogBuffer_Consume(uint8_t* outBuf, uint16_t outBufSize) {
    uint16_t copied = 0;
    uint16_t remaining = (bufferLength < outBufSize) ? bufferLength : outBufSize;
    while (remaining > 0) {
        char a = buffer[bufferPosition++];
        outBuf[copied++] = a;
        if (bufferPosition >= USB_LOG_BUFFER_SIZE) {
            bufferPosition = 0;
        }
        remaining--;
        bufferLength--;
    }
    if (copied < outBufSize) {
        outBuf[copied] = 0;
    }
    updateNonemptyFlag();
    return copied;
}

void UsbLogBuffer_GetFill(uint16_t* occupied, uint16_t* size) {
    *occupied = bufferLength;
    *size = USB_LOG_BUFFER_SIZE;
}

void UsbLogBuffer_SnapToStatusBuffer(void) {
    StateWormhole_Open();
    StateWormhole.persistStatusBuffer = true;

    uint16_t pos = bufferPosition;
    uint16_t len = bufferLength;

    for (uint16_t i = 0; i < len; i++) {
        char c = buffer[pos];
        Macros_SanitizedPut(&c, &c + 1);
        pos = (pos + 1) % USB_LOG_BUFFER_SIZE;
    }
}
