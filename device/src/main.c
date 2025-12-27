#include "main.h"
#include "bt_advertise.h"
#include "messenger_queue.h"
#include "nus_client.h"
#include "nus_server.h"
#include "bt_manager.h"
#include "config_parser/config_globals.h"
#include "ledmap.h"
#include "pin_wiring.h"
#include "round_trip_test.h"
#include "shared/attributes.h"
#include "zephyr/kernel.h"
#include "zephyr/storage/flash_map.h"
#include "keyboard/key_scanner.h"
#include "keyboard/leds.h"
#include "keyboard/oled/oled.h"
#include "keyboard/charger.h"
#include "keyboard/spi.h"
#include "keyboard/uart_bridge.h"
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
#include "macros/vars.h"
#include "thread_stats.h"
#include "power_mode.h"
#include "mouse_controller.h"
#include "wormhole.h"
#include "power_mode.h"
#include "proxy_log_backend.h"
#include "logger_priority.h"
#include "keyboard/uart_modules.h"

#if DEVICE_IS_KEYBOARD
#include "keyboard/battery_unloaded_calculator.h"
#include "keyboard/battery_percent_calculator.h"
#endif


/**
 * 5.1mA - base. Isn't that quite a lot? (Although this is on 5V usb - will be less on battery)
 *-----------
 * uart shell  +???
 * Leds        +1.5mA
 * USB         +2.2mA
 * bridge uart +1.6mA
 * i2c         +0.6mA
 *------------
 * total 11 mA
 * -----------
 *  This was measured on usb, right half, without module, jtag connected (and adding some 1-2 mA to the draw)
 */

k_tid_t Main_ThreadId = 0;

static void sleepTillNextMs() {
    static uint64_t wakeupTimeUs = 0;
    const uint64_t minSleepTime = 100;
    uint64_t currentTimeUs = k_cyc_to_us_near64(k_cycle_get_32());

    wakeupTimeUs = MIN(currentTimeUs,wakeupTimeUs)+1000;

    if (currentTimeUs < wakeupTimeUs) {
        uint64_t timeToSleep = MAX(wakeupTimeUs-currentTimeUs, minSleepTime);
        k_usleep(timeToSleep);
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
    int32_t diff = nextEventTime - Timer_GetCurrentTime();

    Trace(')');

    k_sem_take(&mainWakeupSemaphore, K_NO_WAIT);
    bool haveMoreWork = (EventScheduler_Vector & EventVector_UserLogicUpdateMask);
    if (haveMoreWork) {
        LOG_SCHEDULE( EventVector_ReportMask("Continuing immediately because of: ", EventScheduler_Vector & EventVector_UserLogicUpdateMask););
        EVENTLOOP_TIMING(printk("Continuing immediately\n"));
        // Mouse keys don't like being called twice in one second for some reason
        k_sem_give(&mainWakeupSemaphore);
        sleepTillNextMs();
        Trace('(');
        return;
    } else if (eventIsValid) {
        EVENTLOOP_TIMING(printk("Sleeping for %d\n", diff));
        k_sem_take(&mainWakeupSemaphore, K_MSEC(diff));
        // k_sleep(K_MSEC(diff));
    } else {
        EVENTLOOP_TIMING(printk("Sleeping forever\n"));
        k_sem_take(&mainWakeupSemaphore, K_FOREVER);
        // k_sleep(K_FOREVER);
    }
    Trace('(');
}

//TODO: inline this
void Main_Wake() {
    k_sem_give(&mainWakeupSemaphore);
    // k_wakeup(Main_ThreadId);
}

static void detectSpinningEventLoop() {
    if (!Cfg.DevMode) {
        return;
    }

    const uint16_t maxEventsPerSecond = 300; //allow 5ms macro wait loops
    static uint32_t thisCheckTime = 0;
    static uint16_t eventCount = 0;
    static uint16_t spinPeriods = 0;

    if (thisCheckTime == Timer_GetCurrentTime() / 1024) {
        bool isMouseEvent = EventScheduler_Vector & (EventVector_MouseKeys | EventVector_MouseController | EventVector_SendUsbReports | EventVector_MacroEngine);
        if (!DEVICE_IS_UHK80_RIGHT || !isMouseEvent) {
            eventCount++;
        }
    } else {
        if (eventCount > maxEventsPerSecond) {
            spinPeriods++;
            if (spinPeriods > 30) {
#ifdef __ZEPHYR__
                device_id_t target = DeviceId_Uhk80_Right;
#else
                device_id_t target = DEVICE_ID;
#endif
                LogTo(target, LogTarget_Uart | LogTarget_ErrorBuffer, "Looks like the event loop is spinning quite a lot: %u events in the last second.\n", eventCount);
                LogTo(target, LogTarget_Uart | LogTarget_ErrorBuffer, "EV: 0x%x\n", StateWormhole.traceBuffer.eventVector);
                spinPeriods = 0;
            }
        } else {
            spinPeriods = 0;
        }
        thisCheckTime = Timer_GetCurrentTime() / 1024;
        eventCount = 0;
    }
}

void mainRuntime(void) {
    Main_ThreadId = k_current_get();
    printk("----------\n" DEVICE_NAME " started\n");
    // 5.1mA

    {
        flash_area_open(FLASH_AREA_ID(hardware_config_partition), &hardwareConfigArea);
        flash_area_open(FLASH_AREA_ID(user_config_partition), &userConfigArea);
    }

    if (!DEVICE_IS_UHK_DONGLE) {
        InitSpi();

        InitLeds(); // +1.5mA; 6.6mA

#if DEVICE_HAS_OLED
        InitOled();
#endif // DEVICE_HAS_OLED

#if DEVICE_HAS_MERGE_SENSOR
        MergeSensor_Init();
#endif // DEVICE_HAS_MERGE_SENSOR

        InitKeyScanner();

    }

    bt_enable(NULL); // has to be before InitSettings

    // has to be after bt_enable; has to be before ApplyConfig
    InitSettings();

    // needs to be after InitSettings
    if (DEVICE_IS_UHK80_RIGHT) {
        PinWiring_UninitShell();
        PinWiring_Suspend();

        PinWiring_SelectRouting();
        PinWiring_ConfigureRouting();

        PinWiring_Resume();

        ReinitShell();
    }

    // Needs to be after ReinitShell, probably
    InitShellCommands();
    InitProxyLogBackend();

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
            if (factoryMode || UsbCommand_ValidateAndApplyConfigSync(NULL, NULL) != UsbStatusCode_Success) {
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

    // 6.6mA

    HID_SetGamepadActive(false);
    USB_Enable(); // +2.2mA, 8.8mA; has to be after USB_SetSerialNumber

    // 8.8mA

    if (LastRunWasCrash) {
        printk("CRASH DETECTED, waiting for 5 seconds to allow Agent to reenumerate\n");
        k_sleep(K_MSEC(5*1000));
    }

    // 8.8mA

    // Uart has to be enabled only after we have given Agent a chance to reenumarate into bootloader after a crash
    if (!DEVICE_IS_UHK_DONGLE) {
        InitUartModules(); // +1.6mA
        InitUartBridge(); // +1.6mA
        InitZephyrI2c(); // +0.6mA
    }

    // 11mA

    // Uart has to be enabled only after we have given Agent a chance to reenumarate into bootloader after a crash
    // has to be after InitSettings
    BtManager_InitBt();
    BtManager_StartBt();

    if (!DEVICE_IS_UHK_DONGLE) {
        InitCharger(); // has to be after usb initialization
    }

    EventVector_Init();

    Messenger_Init();

    StateSync_Init();

    RoundTripTest_Init();

    if (DEBUG_RUN_TESTS) {
        MacroVariables_RunTests();
#if DEVICE_IS_KEYBOARD
        MouseController_RunTests();
        BatteryCalculator_RunTests();
        BatteryCalculator_RunPercentTests();
#endif
    }

    // Call after all threads have been created
    ThreadStats_Init();

#if DEVICE_IS_UHK80_RIGHT
    while (true)
    {
        Messenger_ProcessQueue();
        if (EventScheduler_Vector & EventVector_UserLogicUpdateMask) {
            EVENTLOOP_TIMING(EventloopTiming_Start());
            RunUserLogic();
            EVENTLOOP_TIMING(EventloopTiming_End());
        }
        scheduleNextRun();
        detectSpinningEventLoop();
        UserLogic_LastEventloopTime = Timer_GetCurrentTime();
    }
#else
    while (true)
    {
        Messenger_ProcessQueue();
        RunUhk80LeftHalfLogic();
        scheduleNextRun();
        detectSpinningEventLoop();
    }
#endif
}

int main(void) {
    power_mode_t mode = PowerMode_Awake;

    Trace_Init();
    if (StateWormhole_IsOpen()) {
        printk("Wormhole is open, reboot to power mode %d %d\n", StateWormhole.rebootToPowerMode, StateWormhole.restartPowerMode);
        if (StateWormhole.rebootToPowerMode) {
            mode = StateWormhole.restartPowerMode;
            StateWormhole.restartPowerMode = PowerMode_Awake;
        }
        if (StateWormhole.devMode) {
            MacroStatusBuffer_InitFromWormhole();
        } else {
            MacroStatusBuffer_InitNormal();
        }
        StateWormhole_Clean();
    } else {
        printk("Wormhole is closed\n");
        MacroStatusBuffer_InitNormal();
        StateWormhole_Clean();
        StateWormhole_Open();
    }

    if (mode != PowerMode_Awake) {
        LogU("Restarted, sinking into mode %d!\n", mode);
        k_sleep(K_MSEC(1000));
        PowerMode_RestartedTo(mode);
    }

    LogU("Going to resume!\n");

    mainRuntime();
}
