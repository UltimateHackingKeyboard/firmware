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
#include "led_display.h"
#include "touchpad_driver.h"

void TouchpadDriver_Init(uint8_t uhkModuleDriverId)
{
}

typedef struct {
    bool singleTap: 1;
    bool tapAndHold: 1;
    uint8_t unused: 6;
} gesture_events0_t;

typedef struct {
    bool twoFingerTap : 1;
    bool scroll : 1;
    bool zoom : 1;
    uint8_t unused: 5;
} gesture_events1_t;

static gesture_events0_t gestureEvents0;
static gesture_events1_t gestureEvents1;

uint8_t address = I2C_ADDRESS_RIGHT_IQS5XX_FIRMWARE;
touchpad_events_t TouchpadEvents;
uint8_t phase = 0;
static uint8_t enableEventMode[] = {0x05, 0x8f, 0x07};
static uint8_t getGestureEvents0[] = {0x00, 0x0d};
static uint8_t getGestureEvents1[] = {0x00, 0x0e};
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
            status = I2cAsyncWrite(address, getGestureEvents0, sizeof(getGestureEvents0));
            phase = 2;
            break;
        }
        case 2: {
            status = I2cAsyncRead(address, (uint8_t*)&gestureEvents0, 1);
            phase = 3;
            break;
        }
        case 3: {
            status = I2cAsyncWrite(address, getGestureEvents1, sizeof(getGestureEvents1));
            phase = 4;
            break;
        }
        case 4: {
            status = I2cAsyncRead(address, (uint8_t*)&gestureEvents1, 1);
            phase = 5;
            break;
        }
        case 5: {
            status = I2cAsyncWrite(address, getRelativePixelsXCommand, sizeof(getRelativePixelsXCommand));
            phase = 6;
            break;
        }
        case 6: {
            status = I2cAsyncRead(address, buffer, 2);
            deltaX = (int16_t)(buffer[1] | buffer[0]<<8);
            phase = 7;
            break;
        }
        case 7: {
            status = I2cAsyncWrite(address, getRelativePixelsYCommand, sizeof(getRelativePixelsYCommand));
            phase = 8;
            break;
        }
        case 8: {
            status = I2cAsyncRead(address, buffer, 2);
            deltaY = (int16_t)(buffer[1] | buffer[0]<<8);
            phase = 9;
            break;
        }
        case 9: {
            TouchpadEvents.singleTap |= gestureEvents0.singleTap;
            TouchpadEvents.twoFingerTap |= gestureEvents1.twoFingerTap;
            TouchpadEvents.tapAndHold = gestureEvents0.tapAndHold;
            if (gestureEvents1.scroll) {
                TouchpadEvents.wheelX -= deltaX;
                TouchpadEvents.wheelY += deltaY;
            } else if (gestureEvents1.zoom) {
                TouchpadEvents.zoomLevel += deltaY;
            } else {
                TouchpadEvents.x -= deltaX;
                TouchpadEvents.y += deltaY;
            }
//            LedDisplay_SetIcon(LedDisplayIcon_Adaptive, gestureEvents1.scroll);
//            LedDisplay_SetIcon(LedDisplayIcon_Adaptive, gestureEvents1.zoom);
            status = I2cAsyncWrite(address, closeCommunicationWindow, sizeof(closeCommunicationWindow));
            phase = 1;
            break;
        }
    }

    return status;
}

void TouchpadDriver_Disconnect(uint8_t uhkModuleDriverId)
{
    TouchpadEvents.x = 0;
    TouchpadEvents.y = 0;
}
