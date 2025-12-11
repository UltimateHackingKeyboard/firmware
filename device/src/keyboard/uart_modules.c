#include "uart_modules.h"
#include "slave_scheduler.h"
#include "keyboard/uart_link.h"
#include "shared/uart_parser.h"
#include "slave_scheduler.h"
#include "slave_drivers/uhk_module_driver.h"

#define THREAD_STACK_SIZE 1000
#define THREAD_PRIORITY -1

static K_THREAD_STACK_DEFINE(stack_area, THREAD_STACK_SIZE);
static struct k_thread thread_data;

typedef struct {
    uart_link_t core;
    uart_parser_t parser;
    uint8_t rxBuffer[MAX_LINK_PACKET_LENGTH];
} uart_state_t;

static uart_state_t modulesState;

static void receivePacket(void *state, uart_control_t messageKind, const uint8_t* data, uint16_t len) {
    printk("Received uart packet (%d):   ", messageKind);
    for (uint16_t i = 0; i < len; i++) {
        printk("%02X ", data[i]);
    }

    uart_state_t *uartState = (uart_state_t *)state;
    switch (messageKind) {
        case UartControl_Ack:
        case UartControl_Nack:
        case UartControl_Ping:
        case UartControl_ValidMessage:
        case UartControl_InvalidMessage: {
        case UartControl_Unexpected:
            UartLink_Reset(&uartState->core);
            break;
        }
    }
}

static void uartLoop(void *arg1, void *arg2, void *arg3) {

    while (true) {
        printk("Scheduling a transfer!\n");
        SlaveScheduler_ScheduleSingleTransfer(UhkModuleDriverId_RightModule);
        k_sleep(K_MSEC(1000));
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

    UartParser_SetRxBuffer(&uartState->parser, uartState->rxBuffer);
}

void InitUartModules(void) {

    if (PinWiringConfig->device_uart_modules != NULL && PinWiringConfig->device_uart_modules->device != NULL) {
        initUart(
                &modulesState,
                PinWiringConfig->device_uart_modules
                );

        k_thread_create(
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
    uart_state_t *uartState = &modulesState;

    if (uartState == NULL || uartState->core.device == NULL) {
        return -1;
    }

    UartLink_LockBusy(&uartState->core);

    // Call this only after we have taken the semaphore.
    //Resend_RegisterMessageAndUpdateWatermarks(msg);


    UartParser_StartMessage(&uartState->parser);

    UartParser_AppendEscapedTxBytes(&uartState->parser, msg->data, msg->length);

    UartParser_FinalizeMessage(&uartState->parser);

    int err = UartLink_Send(&uartState->core, uartState->parser.txBuffer, uartState->parser.txPosition);

    return err;
}
