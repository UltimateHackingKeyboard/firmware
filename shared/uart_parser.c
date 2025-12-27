#include "uart_parser.h"
#include "crc16.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "uart_defs.h"

#if !defined(__ZEPHYR__) && defined(MODULE_ID)
    #include "shared/module/uart_link.h"
#endif

#ifdef DEVICE_ID
#include "logger.h"
#else
#define LogU(...)
#endif

#define CRC_SALT 0x1234
#define CRC_LEN UART_CRC_LEN

static void appendRxByte(uart_parser_t *uartState, uint8_t byte) {
    if (uartState->rxPosition < CRC_LEN) {
        uartState->rxCrcBuffer[uartState->rxPosition++] = byte;
    } else if (uartState->rxPosition - CRC_LEN < uartState->rxLength) {
        uartState->rxBuffer[uartState->rxPosition++ - CRC_LEN] = byte;
    } else {
        LogU("Uart error: too long message [%i, %i, ... %i]\n", uartState->rxPosition, uartState->rxBuffer[0], uartState->rxBuffer[1], byte);
        uartState->receiveMessage(uartState, UartControl_Unexpected, NULL, 0);
    }
}

ATTR_UNUSED static uint16_t get_random(void)
{
    static uint16_t lfsr = 0xACE1;  // Non-zero seed
    uint16_t bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5)) & 1;
    lfsr = (lfsr >> 1) | (bit << 15);
    return lfsr;
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

static void processIncomingByte(uart_parser_t *uartState, uint8_t byte) {
#if DEBUG_STRESS_UART
    uint16_t r1 = get_random();
    uint8_t r2 = get_random();

    if (r1 < 128) {
        LogU("Oops!\n");
        byte = byte ^ r2;
    }
#endif


    switch (byte) {
        case UartControlByte_Ack:
            if (uartState->receivingMessage) {
                goto msg_byte;
            }

            uartState->receiveMessage(uartState->userArg, UartControl_Ack, NULL, 0);
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

                uint16_t len = uartState->rxPosition;
                uint8_t* data = uartState->rxBuffer;

                if (len >= CRC_LEN && isCrcValid(uartState, data, len - CRC_LEN)) {
                    uartState->receiveMessage(uartState->userArg, UartControl_ValidMessage, data, len - CRC_LEN);
                } else {
                    uartState->receiveMessage(uartState->userArg, UartControl_InvalidMessage, data, len - CRC_LEN);
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
        case UartControlByte_Nack:
        case UartControlByte_Ping:
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
    uartState->receiveMessage = receiveMessage;
    uartState->userArg = userArg;
}

void UartParser_SetRxBuffer(uart_parser_t *uartState, uint8_t* buffer, uint16_t length) {
    uartState->rxBuffer = buffer;
    uartState->rxPosition = 0;
    uartState->rxLength = length;
    uartState->escaping = false;
    uartState->receivingMessage = false;
}

void UartParser_SetTxBuffer(uart_parser_t *uartState, uint8_t* buffer, uint16_t length) {
    uartState->txBuffer = buffer;
    uartState->txPosition = 0;
    uartState->txLength = length;
}
