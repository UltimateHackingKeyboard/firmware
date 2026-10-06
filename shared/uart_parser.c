#include "uart_parser.h"
#include "crc16.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "uart_defs.h"

#if !defined(__ZEPHYR__) && defined(MODULE_ID)
    #include "shared/module/uart_link.h"
#endif

#include "device.h"

#ifdef DEVICE_ID
#include "logger.h"
#include "debug.h" // DEBUG_STRESS_UART
#else
#define LogU(...)
#define DEBUG_STRESS_UART false
#endif

#define STRESS_RANDOM_RANGE 65536
// Do NOT change these - test_link.c statistics are calibrated against them.
#define STRESS_BYTE_FAULT_RECIPROCAL 512
#define STRESS_ACK_DROP_RECIPROCAL 16

#define STRESS_BYTE_FAULT_THRESHOLD (STRESS_RANDOM_RANGE / STRESS_BYTE_FAULT_RECIPROCAL)
#define STRESS_ACK_DROP_THRESHOLD (STRESS_RANDOM_RANGE / STRESS_ACK_DROP_RECIPROCAL)

#define CRC_SALT 0x1234
#define CRC_LEN UART_CRC_LEN

static void appendRxByte(uart_parser_t *uartState, uint8_t byte) {
    if (uartState->rxPosition < CRC_LEN) {
        uartState->rxCrcBuffer[uartState->rxPosition++] = byte;
    } else if (uartState->rxPosition - CRC_LEN < uartState->rxLength) {
        uartState->rxBuffer[uartState->rxPosition++ - CRC_LEN] = byte;
    } else if (!uartState->rxTooLong) {
        uartState->rxTooLong = true;
        LogU("Uart error: too long message, discarding [len %i: src %i, dst %i, msgId %i, propId %i]\n",
            uartState->rxLength, uartState->rxBuffer[0], uartState->rxBuffer[1], uartState->rxBuffer[3], uartState->rxBuffer[4]);
    }
}

// xorshift32, top half of each draw. Not the one-bit-per-call LFSR this used to be: its
// consecutive draws shared all but one bit, so the drop rolls of acks arriving back to back -
// exactly what a retry loop produces - were correlated, making two and three dropped acks in
// a row about 2x and 4x likelier than their nominal rate.
ATTR_UNUSED static uint16_t get_random(void)
{
    static uint32_t state = 0x2545F491;  // Non-zero seed
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return (uint16_t)(state >> 16);
}

static bool isCrcValid(uart_parser_t *uartState, const uint8_t* buf, uint16_t len) {
    uint16_t crc = (uartState->rxCrcBuffer[0] | (uartState->rxCrcBuffer[1] << 8)) ^ CRC_SALT;

    crc16_message_t msg = {
        .length = len,
        .crc = crc,
        .data = buf
    };

    return CRC16_IsMessageValidExt(&msg);
}

// Default comes from the build flag, so a stress build still corrupts from boot, but tests
// can switch it on for their duration and off again afterwards.
bool UartStress_Active = DEBUG_STRESS_UART;

// Returns true when the byte should be swallowed. Mutates `byte` in place otherwise.
static bool stressByte(uart_parser_t *uartState, uint8_t *byte) {
    uint16_t corruptRoll = get_random();
    uint8_t corruptMask = get_random();
    uint16_t dropRoll = get_random();

    if (corruptRoll < STRESS_BYTE_FAULT_THRESHOLD) {
        LogU("UartStress: Oops!\n");
        *byte = *byte ^ corruptMask;
    }

    if (dropRoll < STRESS_BYTE_FAULT_THRESHOLD) {
        return true;
    }

    // More dropped acks, more fun: CRC mutation alone never reaches the resend path.
    bool isAckLike = *byte == UartControlByte_Ack || *byte == UartControlByte_Ack0
        || *byte == UartControlByte_Ack1 || *byte == UartControlByte_Nack;

    return dropRoll < STRESS_ACK_DROP_THRESHOLD && isAckLike && !uartState->receivingMessage;
}

static void processIncomingByte(uart_parser_t *uartState, uint8_t byte) {
    if (UartStress_Active && stressByte(uartState, &byte)) {
        return;
    }


    switch (byte) {
        case UartControlByte_Ack:
            if (uartState->receivingMessage) {
                goto msg_byte;
            }

            uartState->receiveMessage(uartState->userArg, UartControl_Ack, NULL, 0);
            break;
        case UartControlByte_Ack0:
            if (uartState->receivingMessage) {
                goto msg_byte;
            }

            uartState->receiveMessage(uartState->userArg, UartControl_Ack0, NULL, 0);
            break;
        case UartControlByte_Ack1:
            if (uartState->receivingMessage) {
                goto msg_byte;
            }

            uartState->receiveMessage(uartState->userArg, UartControl_Ack1, NULL, 0);
            break;
        case UartControlByte_Nack:
            if (uartState->receivingMessage) {
                goto msg_byte;
            }

            uartState->receiveMessage(uartState->userArg, UartControl_Nack, NULL, 0);
            break;
        case UartControlByte_Ping:
            // Always accept pings.
            //
            // Reestablishing connection is expensive, so in case of bad quality
            // uart connection, once successful, we don't want to loose it just
            // because of a broken packet frame.
            uartState->receiveMessage(uartState->userArg, UartControl_Ping, NULL, 0);

            if (uartState->receivingMessage) {
                goto msg_byte;
            }
            break;
        case UartControlByte_End: {
                if (uartState->escaping) {
                    goto msg_byte;
                }

                uartState->receivingMessage = false;

                if (uartState->rxTooLong) {
                    uartState->rxTooLong = false;
                    break;
                }

                uint16_t frameLen = uartState->rxPosition;
                uint16_t dataLen = frameLen - CRC_LEN; // crc and data are saved into different buffers
                uint8_t* data = uartState->rxBuffer;

                if (frameLen >= CRC_LEN && isCrcValid(uartState, data, dataLen)) {
                    uartState->receiveMessage(uartState->userArg, UartControl_ValidMessage, data, dataLen);
                } else {
                    uartState->receiveMessage(uartState->userArg, UartControl_InvalidMessage, data, dataLen);
                }
            }
            break;
        case UartControlByte_Escape:
            if (uartState->escaping) {
                goto msg_byte;
            }
            uartState->escaping = true;
            break;
        case UartControlByte_Start:
            if (uartState->escaping) {
                goto msg_byte;
            }
            uartState->receivingMessage = true;
            uartState->rxPosition = 0;
            uartState->rxTooLong = false;
            break;
        case UartControlByte_Wake:
            // Carries no meaning and must be swallowed silently - reporting it as
            // Unexpected would tear RX down exactly when the frame behind it is arriving.
            // It is escaped on TX, so an unescaped one here is never payload.
            if (uartState->escaping) {
                goto msg_byte;
            }
            break;
msg_byte:
        default:
            uartState->escaping = false;
            if (uartState->receivingMessage) {
                appendRxByte(uartState, byte);
            } else {
                uartState->receiveMessage(uartState->userArg, UartControl_Unexpected, NULL, 0);
            }
            break;
    }
}

void UartParser_ProcessIncomingBytes(void *state, const uint8_t* data, uint16_t len) {
    uart_parser_t *uartState = (uart_parser_t *)state;
    for (uint16_t i = 0; i < len; i++) {
        processIncomingByte(uartState, data[i]);
    }
}

void appendByte(uart_parser_t *uartState, uint8_t byte) {
    if (uartState->txPosition < uartState->txLength) {
        uartState->txBuffer[uartState->txPosition++] = byte;
    } else {
        LogU("Uart error: too long message in tx buffer\n");
    }
}

// Used to retroactively set crc
static void setEscapedTxByte(uart_parser_t *uartState, uint8_t idx, uint8_t byte, uint8_t escape) {
    uartState->txBuffer[idx] = escape;
    uartState->txBuffer[idx+1] = byte;
}


static void escapeAndAppend(uart_parser_t *uartState, uint8_t byte) {
    switch (byte) {
        case UartControlByte_Start:
        case UartControlByte_End:
        case UartControlByte_Escape:
        case UartControlByte_Ack:
        case UartControlByte_Ack0:
        case UartControlByte_Ack1:
        case UartControlByte_Nack:
        case UartControlByte_Ping:
        case UartControlByte_Wake:
            appendByte(uartState, UartControlByte_Escape);
            appendByte(uartState, byte);
            break;
        default:
            appendByte(uartState, byte);
            break;
    }
}

void UartParser_AppendEscapedTxBytes(uart_parser_t *uartState, const uint8_t* data, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        escapeAndAppend(uartState, data[i]);
    }

    crc16_update(&uartState->crcState, data, len);
}


static void finalizeCrc(uart_parser_t *uartState, crc16_data_t* crcState) {
    uint16_t crc;
    crc16_finalize(crcState, &crc);
    crc = crc ^ CRC_SALT;
    setEscapedTxByte(uartState, 1, crc & 0xFF, UartControlByte_Escape);
    setEscapedTxByte(uartState, 3, crc >> 8, UartControlByte_Escape);
}


void UartParser_FinalizeMessage(uart_parser_t *uartState) {
    appendByte(uartState, UartControlByte_End);
    finalizeCrc(uartState, &uartState->crcState);
}

void UartParser_StartMessage(uart_parser_t *uartState) {
    appendByte(uartState, UartControlByte_Start);
    uartState->txPosition = UART_LINK_CRC_BUF_LEN+1;

    crc16_init(&uartState->crcState);
}

void UartParser_InitParser(
    uart_parser_t* uartState,
    void (*receiveMessage)(void* state, uart_control_t messageKind, const uint8_t* rxBuffer, uint16_t len),
    void* userArg
) {
    uartState->rxPosition = 0;
    uartState->rxBuffer = NULL;
    uartState->txPosition = 0;
    uartState->receivingMessage = false;
    uartState->escaping = false;
    uartState->rxTooLong = false;
    uartState->receiveMessage = receiveMessage;
    uartState->userArg = userArg;
}

void UartParser_SetRxBuffer(uart_parser_t *uartState, uint8_t* buffer, uint16_t length) {
    uartState->rxBuffer = buffer;
    uartState->rxPosition = 0;
    uartState->rxLength = length;
    uartState->escaping = false;
    uartState->receivingMessage = false;
    uartState->rxTooLong = false;
}

void UartParser_SetTxBuffer(uart_parser_t *uartState, uint8_t* buffer, uint16_t length) {
    uartState->txBuffer = buffer;
    uartState->txPosition = 0;
    uartState->txLength = length;
}
