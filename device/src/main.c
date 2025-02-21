#include "main.h"
#include "bt_advertise.h"
#include "messenger_queue.h"
#include "nus_client.h"
#include "nus_server.h"
#include "bt_manager.h"
#include "config_parser/config_globals.h"
#include "ledmap.h"
#include "round_trip_test.h"
#include "shared/attributes.h"
#include "zephyr/kernel.h"
#include "zephyr/storage/flash_map.h"
#include "keyboard/key_scanner.h"
#include "keyboard/leds.h"
#include "keyboard/oled/oled.h"
#include "keyboard/charger.h"
#include "keyboard/spi.h"
#include "keyboard/uart.h"
#include "keyboard/i2c.h"
#include "peripherals/merge_sensor.h"
#include "shell.h"
#include "device.h"
#include "usb/usb.h"
#include "bt_conn.h"
#include "settings.h"
#include "flash.h"
#include "usb_commands/usb_command_apply_config.h"
#include "macros/shortcut_parser.h"
#include "macros/keyid_parser.h"
#include "macros/core.h"
#include "timer.h"
#include "user_logic.h"
#include "config_manager.h"
#include "messenger.h"
#include "led_manager.h"
#include "debug.h"
#include "state_sync.h"
#include "keyboard/charger.h"
#include <stdint.h>
#include "dongle_leds.h"
#include "debug_eventloop_timing.h"
#include <zephyr/drivers/gpio.h>
#include "dongle_leds.h"
#include "usb_protocol_handler.h"
#include "trace.h"
#include "thread_stats.h"

k_tid_t Main_ThreadId = 0;

static void sleepTillNextMs() {
    static uint64_t wakeupTimeUs = 0;
    const uint64_t minSleepTime = 100;
    uint64_t currentTimeUs = k_cyc_to_us_near64(k_cycle_get_32());

    wakeupTimeUs = wakeupTimeUs+1000;

    if (currentTimeUs < wakeupTimeUs) {
        k_usleep(MAX(wakeupTimeUs-currentTimeUs, minSleepTime));
    } else {
        k_usleep(minSleepTime);
        wakeupTimeUs = currentTimeUs;
    }
}


static K_SEM_DEFINE(mainWakeupSemaphore, 1, 1);

static void scheduleNextRun() {
    uint32_t nextEventTime = 0;
    bool eventIsValid = false;
    if (EventScheduler_Vector & EventVector_EventScheduler) {
        nextEventTime = EventScheduler_Process();
        eventIsValid = true;
    }
    CurrentTime = k_uptime_get();
    int32_t diff = nextEventTime - CurrentTime;

    Trace('-');

    k_sem_take(&mainWakeupSemaphore, K_NO_WAIT);
    bool haveMoreWork = (EventScheduler_Vector & EventVector_UserLogicUpdateMask);
    if (haveMoreWork) {
        LOG_SCHEDULE( EventVector_ReportMask("Continuing immediately because of: ", EventScheduler_Vector & EventVector_UserLogicUpdateMask););
        EVENTLOOP_TIMING(printk("Continuing immediately\n"));
        // Mouse keys don't like being called twice in one second for some reason
        Trace_Printf("s31");
        k_sem_give(&mainWakeupSemaphore);
        sleepTillNextMs();
        return;
    } else if (eventIsValid) {
        EVENTLOOP_TIMING(printk("Sleeping for %d\n", diff));
        Trace_Printf("s32");
        k_sem_take(&mainWakeupSemaphore, K_MSEC(diff));
        // k_sleep(K_MSEC(diff));
    } else {
        EVENTLOOP_TIMING(printk("Sleeping forever\n"));
        Trace_Printf("s33");
        k_sem_take(&mainWakeupSemaphore, K_FOREVER);
        // k_sleep(K_FOREVER);
    }
    Trace('+');
}

//TODO: inline this
void Main_Wake() {
    k_sem_give(&mainWakeupSemaphore);
    // k_wakeup(Main_ThreadId);
}

int main(void) {
    Main_ThreadId = k_current_get();
    printk("----------\n" DEVICE_NAME " started\n");

    Trace_Init();

    {
        flash_area_open(FLASH_AREA_ID(hardware_config_partition), &hardwareConfigArea);
        flash_area_open(FLASH_AREA_ID(user_config_partition), &userConfigArea);
    }

    if (!DEVICE_IS_UHK_DONGLE) {
        InitUart();
        InitZephyrI2c();
        InitSpi();

#if DEVICE_HAS_OLED
        InitOled();
#endif // DEVICE_HAS_OLED

        InitLeds();

#if DEVICE_HAS_MERGE_SENSOR
        MergeSensor_Init();
#endif // DEVICE_HAS_MERGE_SENSOR

        InitKeyScanner();

    }

    bt_enable(NULL); // has to be before InitSettings

    // has to be after bt_enable; has to be before ApplyConfig
    InitSettings();

    // read configurations
    {
        InitFlash();
        printk("Reading hardware config\n");
        Flash_ReadAreaSync(hardwareConfigArea, 0, HardwareConfigBuffer.buffer, HARDWARE_CONFIG_SIZE);
        USB_SetSerialNumber(HardwareConfig->uniqueId);

        if (DEVICE_IS_UHK80_LEFT || DEVICE_IS_UHK80_RIGHT) {
            Ledmap_InitLedLayout(); // has to be after hwconfig read
        }

        if (DEVICE_IS_UHK80_RIGHT) {
            printk("Reading user config\n");
            Flash_ReadAreaSync(userConfigArea, 0, StagingUserConfigBuffer.buffer, USER_CONFIG_SIZE);
            printk("Applying user config\n");
            bool factoryMode = false;
            if (factoryMode || UsbCommand_ApplyConfig(NULL, NULL) != UsbStatusCode_Success) {
                UsbCommand_ApplyFactory(NULL, NULL);
            }
            printk("User config applied\n");
            ShortcutParser_initialize();
            KeyIdParser_initialize();
            Macros_Initialize();
        } else {
            UsbCommand_ApplyFactory(NULL, NULL);
        }
    }

    USB_Enable(); // has to be after USB_SetSerialNumber

    // has to be after InitSettings
    BtManager_InitBt();
    BtManager_StartBt();

    if (!DEVICE_IS_UHK_DONGLE) {
        InitCharger(); // has to be after usb initialization
    }

    Messenger_Init();

    StateSync_Init();

    InitShell();

    RoundTripTest_Init();

    // Call after all threads have been created
    ThreadStats_Init();

#if DEVICE_IS_UHK80_RIGHT
    while (true)
    {
        CurrentTime = k_uptime_get();
        Messenger_ProcessQueue();
        if (EventScheduler_Vector & EventVector_UserLogicUpdateMask) {
            EVENTLOOP_TIMING(EVENTLOOP_TIMING(EventloopTiming_Start()));
            RunUserLogic();
            EVENTLOOP_TIMING(EventloopTiming_End());
        }
        scheduleNextRun();
    }
#else
    while (true)
    {
        CurrentTime = k_uptime_get();
        Messenger_ProcessQueue();
        RunUhk80LeftHalfLogic();
        scheduleNextRun();
    }
#endif
}
