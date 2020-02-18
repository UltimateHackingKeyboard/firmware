#include "i2c_addresses.h"
#include "i2c.h"
#include "slave_scheduler.h"
#include "slave_drivers/uhk_module_driver.h"
#include "slave_protocol.h"
#include "peripherals/test_led.h"
#include "bool_array_converter.h"
#include "crc16.h"
#include "key_states.h"
#include "usb_interfaces/usb_interface_mouse.h"

void TouchpadDriver_Init(uint8_t uhkModuleDriverId)
{
}

uint8_t address = I2C_ADDRESS_RIGHT_IQS5XX_FIRMWARE;
usb_mouse_report_t TouchpadUsbMouseReport;
uint8_t phase = 0;
static uint8_t enableEventMode[] = {0x05, 0x8f, 0x07};
static uint8_t getRelativePixelsXCommand[] = {0x00, 0x12};
static uint8_t getRelativePixelsYCommand[] = {0x00, 0x14};
static uint8_t closeCommunicationWindow[] = {0xee, 0xee, 0xee};
static uint8_t buffer[2];
int16_t deltaX;
int16_t deltaY;

status_t TouchpadDriver_Update(uint8_t uhkModuleDriverId)
{
    status_t status = kStatus_Uhk_IdleSlave;

    switch (phase) {
        case 0: {
            status = I2cAsyncWrite(address, enableEventMode, sizeof(enableEventMode));
            phase = 1;
            break;
        }
        case 1: {
            status = I2cAsyncWrite(address, getRelativePixelsXCommand, sizeof(getRelativePixelsXCommand));
            phase = 2;
            break;
        }
        case 2: {
            status = I2cAsyncRead(address, buffer, 2);
            deltaX = (int16_t)(buffer[1] | buffer[0]<<8);
            phase = 3;
            break;
        }
        case 3: {
            status = I2cAsyncWrite(address, getRelativePixelsYCommand, sizeof(getRelativePixelsYCommand));
            phase = 4;
            break;
        }
        case 4: {
            status = I2cAsyncRead(address, buffer, 2);
            deltaY = (int16_t)(buffer[1] | buffer[0]<<8);
            TouchpadUsbMouseReport.x -= deltaX;
            TouchpadUsbMouseReport.y += deltaY;
            phase = 5;
            break;
        }
        case 5: {
            status = I2cAsyncWrite(address, closeCommunicationWindow, sizeof(closeCommunicationWindow));
            phase = 1;
            break;
        }
    }

    return status;
}

void TouchpadDriver_Disconnect(uint8_t uhkModuleDriverId)
{
    TouchpadUsbMouseReport.x = 0;
    TouchpadUsbMouseReport.y = 0;
}
