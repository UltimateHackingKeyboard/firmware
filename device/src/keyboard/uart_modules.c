#include "uart_modules.h"
#include "slave_scheduler.h"
#include "keyboard/uart_link.h"
#include "shared/uart_parser.h"
#include "slave_scheduler.h"
#include "slave_drivers/uhk_module_driver.h"
#include "i2c_compatibility.h"
#include <zephyr/drivers/pinctrl.h>
#include "pin_wiring.h"
#include "uart_defs.h"
#include "zephyr/kernel.h"

#define THREAD_STACK_SIZE 1000
#define THREAD_PRIORITY -1

static K_THREAD_STACK_DEFINE(stack_area, THREAD_STACK_SIZE);
static struct k_thread thread_data;
static k_tid_t tid = 0;

typedef struct {
    uart_link_t core;
    uart_parser_t parser;
    uint8_t rxBuffer[UART_MAX_MODULE_PAYLOAD_LENGTH];
    uint8_t txBuffer[UART_MAX_MODULE_PAYLOAD_LENGTH];
} uart_state_t;

static uart_state_t modulesState;

bool shouldProcess = false;
status_t lastStatus = kStatus_Fail;

static K_SEM_DEFINE(newMessageSemaphore, 1, 1);

static void receivePacket(void *state, uart_control_t messageKind, const uint8_t* data, uint16_t len) {
    uhk_module_state_t *uhkModuleState = UhkModuleStates + UhkModuleDriverId_RightModule;
    status_t status;
    switch (messageKind) {
        case UartControl_ValidMessage:
            memcpy(uhkModuleState->rxMessage.data, data, len);
            uhkModuleState->rxMessage.length = len;
            uhkModuleState->rxMessage.crc = 1;
            status = kStatus_Success;
            break;
        case UartControl_InvalidMessage:
            uhkModuleState->rxMessage.crc = 0;
            status = kStatus_Success;
            break;
        default:
            status = kStatus_Fail;
            break;
    }

    lastStatus = status;

    if (status == kStatus_Success) {
        k_sem_give(&newMessageSemaphore);
    }
}

static void uartLoop(void *arg1, void *arg2, void *arg3) {
    while (true) {
        SlaveScheduler_FinalizeTransfer(UhkModuleDriverId_RightModule, lastStatus);
        lastStatus = kStatus_Fail;
        SlaveScheduler_ScheduleSingleTransfer(UhkModuleDriverId_RightModule);
        k_sem_take(&newMessageSemaphore, K_MSEC(UART_MODULE_TIMEOUT_MS));
    }
}

static void initUart(
        uart_state_t *uartState,
        const pin_wiring_dev_t* device
) {
    if (device == NULL || device->device == NULL) {
        return;
    }

    UartLink_Init(&uartState->core, device->device, UartParser_ProcessIncomingBytes, (void*)&uartState->parser);
    UartParser_InitParser(&uartState->parser, &receivePacket, (void*)uartState);

    UartParser_SetRxBuffer(&uartState->parser, uartState->rxBuffer, UART_MAX_MODULE_PAYLOAD_LENGTH);
    UartParser_SetTxBuffer(&uartState->parser, uartState->txBuffer, UART_MAX_MODULE_SERIALIZED_MESSAGE_LENGTH);
}

void InitUartModules(void) {

    if (PinWiringConfig->device_uart_modules != NULL && PinWiringConfig->device_uart_modules->device != NULL) {
        initUart(
                &modulesState,
                PinWiringConfig->device_uart_modules
                );

        tid = k_thread_create(
                &thread_data, stack_area,
                K_THREAD_STACK_SIZEOF(stack_area),
                uartLoop,
                &modulesState, NULL, NULL,
                THREAD_PRIORITY, 0, K_NO_WAIT
                );
        k_thread_name_set(&thread_data, "uart_modules");
    }

    UartModules_Enable();
}


void UartModules_Enable() {
    UartLink_Enable(&modulesState.core);
}

int Uart_SendModuleMessage(i2c_message_t* msg) {
    uhk_module_state_t *uhkModuleState = UhkModuleStates + UhkModuleDriverId_RightModule;
    uhkModuleState->rxMessage.crc = 0;

    uart_state_t *uartState = &modulesState;

    if (uartState == NULL || uartState->core.device == NULL) {
        return -1;
    }

    UartLink_LockBusy(&uartState->core);

    UartParser_StartMessage(&uartState->parser);
    UartParser_AppendEscapedTxBytes(&uartState->parser, msg->data, msg->length);
    UartParser_FinalizeMessage(&uartState->parser);

    int err = UartLink_Send(&uartState->core, uartState->parser.txBuffer, uartState->parser.txPosition);

    return err;
}
