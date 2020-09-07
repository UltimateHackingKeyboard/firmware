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
#include "touchpad_driver.h"

void TouchpadDriver_Init(uint8_t uhkModuleDriverId)
{
}

typedef struct {
    struct {
        bool singleTap: 1;
        bool tapAndHold: 1;
        uint8_t unused: 6;
    } events0;
    struct {
        bool twoFingerTap : 1;
        bool scroll : 1;
        bool zoom : 1;
        uint8_t unused: 5;
    } events1;
} gesture_events_t;

static gesture_events_t gestureEvents;

uint8_t address = I2C_ADDRESS_RIGHT_IQS5XX_FIRMWARE;
touchpad_events_t TouchpadEvents;
uint8_t phase = 0;
static uint8_t enableEventMode[] = {0x05, 0x8f, 0x07};
static uint8_t getGestureEvents0[] = {0x00, 0x0d};
static uint8_t getRelativePixelsXCommand[] = {0x00, 0x12};
static uint8_t getDeltaValues[] = {0x01, 0xc1};
static uint8_t closeCommunicationWindow[] = {0xee, 0xee, 0xee};
#define deltaRange 59
static uint8_t buffer[4];
static uint8_t buffer2[deltaRange];
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
            status = I2cAsyncRead(address, (uint8_t*)&gestureEvents, sizeof(gesture_events_t));
            phase = 3;
            break;
        }
        case 3: {
            status = I2cAsyncWrite(address, getRelativePixelsXCommand, sizeof(getRelativePixelsXCommand));
            phase = 4;
            break;
        }
        case 4: {
            status = I2cAsyncRead(address, buffer, 4);
            phase = 5;
            break;
        }
        case 5: {
            deltaY = (int16_t)(buffer[1] | buffer[0]<<8);
            deltaX = (int16_t)(buffer[3] | buffer[2]<<8);

            TouchpadEvents.singleTap |= gestureEvents.events0.singleTap;
            TouchpadEvents.twoFingerTap |= gestureEvents.events1.twoFingerTap;

            if (gestureEvents.events1.scroll) {
                TouchpadEvents.wheelX -= deltaX;
                TouchpadEvents.wheelY += deltaY;
            } else if (gestureEvents.events1.zoom) {
                TouchpadEvents.zoomLevel += deltaY;
            } else {
                TouchpadEvents.x -= deltaX;
                TouchpadEvents.y += deltaY;
            }

            status = I2cAsyncWrite(address, closeCommunicationWindow, sizeof(closeCommunicationWindow));
            phase = 6;
            break;
        }
        case 6: {
            status = I2cAsyncWrite(address, getDeltaValues, sizeof(getDeltaValues));
            phase = 7;
            break;
        }
        case 7: {
            status = I2cAsyncRead(address, buffer2, deltaRange);
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
