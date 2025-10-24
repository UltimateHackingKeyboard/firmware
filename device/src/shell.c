#include "shell.h"
#include "bt_conn.h"
#include "device.h"
#include "keyboard/charger.h"
#include "keyboard/leds.h"
#include "keyboard/oled/oled.h"
#include "logger.h"
#include "usb/usb.h"
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/shell/shell.h>
#include "bt_conn.h"
#include "keyboard/charger.h"
#include "ledmap.h"
#include "event_scheduler.h"
#include "host_connection.h"
#include "thread_stats.h"
#include "trace.h"
#include "usb_compatibility.h"
#include "mouse_keys.h"
#include "config_manager.h"
#include <zephyr/shell/shell_backend.h>
#include <zephyr/shell/shell_uart.h>
#include <zephyr/shell/shell.h>
#include "connections.h"
#include "logger_priority.h"
#include "pin_wiring.h"
#include "device.h"
#include "logger.h"
#include "shell_backend_usb.h"
#include "stubs.h"

shell_t Shell = {
    .keyLog = 0,
    .statLog = 0,
    .ledsAlwaysOn = 0,
    .oledEn = 1,
    .sdbState = 1,
};

void list_backends_by_iteration(void) {
    const struct shell *shell;
    size_t idx = 0;
    size_t backendCount = shell_backend_count_get();

    printk("Available shell backends:\n");
    for (size_t i = 0; i < backendCount; i++) {
        shell = shell_backend_get(idx);
        printk("- Backend %zu: %s\n", idx, shell->name);
        idx++;
    }
}

void Shell_Execute(const char *cmd, const char *source) {
    ShellBackend_Exec(cmd, source);
    return;
}

static int cmd_uhk_keylog(const struct shell *shell, size_t argc, char *argv[])
{
    if (argc == 1) {
        shell_fprintf(shell, SHELL_NORMAL, "%i\n", Shell.keyLog);
    } else {
        Shell.keyLog = argv[1][0] == '1';
    }
    return 0;
}
#if !DEVICE_IS_UHK_DONGLE
static int cmd_uhk_statlog(const struct shell *shell, size_t argc, char *argv[])
{
    if (argc == 1) {
        shell_fprintf(shell, SHELL_NORMAL, "%i\n", Shell.statLog);
    } else {
        Shell.statLog = argv[1][0] == '1';
    }
    return 0;
}

static int cmd_uhk_leds(const struct shell *shell, size_t argc, char *argv[])
{
    if (argc == 1) {
        shell_fprintf(shell, SHELL_NORMAL, "%i\n", Shell.ledsAlwaysOn);
    } else {
        Shell.ledsAlwaysOn = argv[1][0] == '1';
    }
    return 0;
}

static int cmd_uhk_sdb(const struct shell *shell, size_t argc, char *argv[])
{
    if (argc == 1) {
        shell_fprintf(shell, SHELL_NORMAL, "%i\n", Shell.sdbState ? 1 : 0);
    } else {
        Shell.sdbState = argv[1][0] == '1';
        gpio_pin_set_dt(&ledsSdbDt, Shell.sdbState);
    }
    return 0;
}

// Charger behavior:
// - If CHARGER_EN=0 or USB is disconnected, then TS reads 6552[0-9] raw value and sometimes drops to 0.
// - If CHARGER_EN=1, USB is connected, and the battery is disconnected, then STAT alternates between 0 and 1 per second

static int cmd_uhk_charger(const struct shell *shell, size_t argc, char *argv[])
{
    if (argc == 1) {
        shell_fprintf(shell, SHELL_NORMAL, "CHARGER_EN: %i | ", Charger_ChargingEnabled ? 1 : 0);
        shell_fprintf(shell, SHELL_NORMAL, "STAT: %i ", gpio_pin_get_dt(&chargerStatDt) ? 1 : 0);

        Charger_PrintState();
    } else {
        bool newChargingEnabled = argv[1][0] == '1';

        Charger_EnableCharging(newChargingEnabled);
    }
    return 0;
}
#endif // !DEVICE_IS_UHK_DONGLE

#if DEVICE_IS_UHK80_RIGHT
static int cmd_uhk_testled(const struct shell *shell, size_t argc, char *argv[])
{
    if (argc == 1 || argv[1][0] == '1') {
        Ledmap_ActivateTestLedMode(true);
    } else {
        Ledmap_ActivateTestLedMode(false);
    }
    return 0;
}
#endif

#if DEVICE_HAS_OLED
static int cmd_uhk_oled(const struct shell *shell, size_t argc, char *argv[])
{
    if (argc == 1) {
        shell_fprintf(shell, SHELL_NORMAL, "%i\n", Shell.oledEn ? 1 : 0);
    } else {
        Shell.oledEn = argv[1][0] == '1';
        gpio_pin_set_dt(&oledEn, Shell.oledEn);
    }
    return 0;
}
#endif

#if DEVICE_HAS_MERGE_SENSOR
static const struct gpio_dt_spec mergeSenseDt = GPIO_DT_SPEC_GET(DT_ALIAS(merge_sense), gpios);

static int cmd_uhk_merge(const struct shell *shell, size_t argc, char *argv[])
{
    shell_fprintf(shell, SHELL_NORMAL, "%i\n", gpio_pin_get_dt(&mergeSenseDt) ? 1 : 0);
    return 0;
}
#endif

static int cmd_uhk_rollover(const struct shell *shell, size_t argc, char *argv[])
{
    if (argc == 1) {
        shell_fprintf(
            shell, SHELL_NORMAL, "%c\n", (HID_GetKeyboardRollover() == Rollover_NKey) ? 'n' : '6');
    } else {
        HID_SetKeyboardRollover((argv[1][0] == '6') ? Rollover_6Key : Rollover_NKey);
    }
    return 0;
}

static int cmd_uhk_gamepad(const struct shell *shell, size_t argc, char *argv[])
{
    if (argc == 1) {
        shell_fprintf(shell, SHELL_NORMAL, "%c\n", HID_GetGamepadActive() ? 'y' : 'n');
    } else {
        HID_SetGamepadActive(argv[1][0] != '0');
    }
    return 0;
}

static int cmd_uhk_passkey(const struct shell *shell, size_t argc, char *argv[])
{
    int err = 0;
    int passkey = shell_strtol(argv[1], 10, &err);
    if (err) {
        shell_error(shell, "Invalid passkey format (%d)\n", err);
    } else {
        num_comp_reply(passkey);
    }
    return 0;
}

static int cmd_uhk_btunpair(const struct shell *shell, size_t argc, char *argv[])
{
    int err = bt_unpair(BT_ID_DEFAULT, NULL);
    printk("unpair operation result: %d\n", err);
    return 0;
}

static int cmd_uhk_connections(const struct shell *shell, size_t argc, char *argv[])
{
    Connections_PrintInfo(LogTarget_Uart);
    return 0;
}

static int cmd_uhk_threads(const struct shell *shell, size_t argc, char *argv[])
{
#if DEBUG_THREAD_STATS
    ThreadStats_Print();
#else
    printk("Thread stats are disabled\n");
#endif
    return 0;
}

static int cmd_uhk_trace(const struct shell *shell, size_t argc, char *argv[])
{
    Trace_Print(LogTarget_Uart, "Triggered by zephyr shell.");
    return 0;
}

static void printMouseKeyState(mouse_kinetic_state_t* state) {

    int16_t multiplier = state->intMultiplier;
    int16_t currentSpeed = state->currentSpeed;
    int16_t targetSpeed = state->targetSpeed;
    int16_t axisSkew = state->axisSkew*100;
    int16_t xSum = state->xSum*100;
    int16_t ySum = state->ySum*100;

    printk(" - isScroll %d\n", state->isScroll);
    printk(" - wasMoveAction %d\n", state->wasMoveAction);
    printk(" - upState %d\n", state->upState);
    printk(" - downState %d\n", state->downState);
    printk(" - leftState %d\n", state->leftState);
    printk(" - rightState %d\n", state->rightState);
    printk(" - prevMouseSpeed %d\n", state->prevMouseSpeed);
    printk(" - intMultiplier %d\n", multiplier);
    printk(" - currentSpeed %d\n", currentSpeed);
    printk(" - targetSpeed %d\n", targetSpeed);
    printk(" - axisSkew*100 %d\n", axisSkew);
    printk(" - initialSpeed %d\n", state->initialSpeed);
    printk(" - acceleration %d\n", state->acceleration);
    printk(" - deceleratedSpeed %d\n", state->deceleratedSpeed);
    printk(" - baseSpeed %d\n", state->baseSpeed);
    printk(" - acceleratedSpeed %d\n", state->acceleratedSpeed);
    printk(" - xSum*100 %d\n", xSum);
    printk(" - ySum*100 %d\n", ySum);
    printk(" - xOut %d\n", state->xOut);
    printk(" - yOut %d\n", state->yOut);
    printk(" - verticalStateSign %d\n", state->verticalStateSign);
    printk(" - horizontalStateSign %d\n", state->horizontalStateSign);
}

static int cmd_uhk_mouseMultipliers(const struct shell *shell, size_t argc, char *argv[]) {
    int16_t horizontalScrollMultiplier = HorizontalScrollMultiplier();
    int16_t verticalScrollMultiplier = VerticalScrollMultiplier();
    printk("h scroll multiplier: %d (x.xx)\n", horizontalScrollMultiplier);
    printk("v scroll multiplier: %d (x.xx)\n", verticalScrollMultiplier);
    printk("accel / deccel states: %d %d\n", ActiveMouseStates[SerializedMouseAction_Accelerate], ActiveMouseStates[SerializedMouseAction_Decelerate]);

    printk("Mouse move states:\n");

    printMouseKeyState(&Cfg.MouseMoveState);

    printk("Mouse scroll states:\n");

    printMouseKeyState(&Cfg.MouseScrollState);

    return 0;
}

static int cmd_uhk_logPriority(const struct shell *shell, size_t argc, char *argv[])
{
    if (argc > 1 && argv[1][0] == 'h') {
        Logger_SetPriority(true);
        shell_fprintf(shell, SHELL_NORMAL, "Log priority set to high.\n");
    } else {
        Logger_SetPriority(false);
        shell_fprintf(shell, SHELL_NORMAL, "Log priority set to low.\n");
    }
    return 0;
}

static int reinitShell(const struct device *const dev)
{
    int ret;
    const struct shell *sh = NULL;

    sh = shell_backend_uart_get_ptr();

    if (!sh) {
        LogS("Shell backend not found\n");
        return -ENODEV;
    }

    if (!dev) {
        LogS("Shell device is NULL\n");
        return -ENODEV;
    }
    const struct device *const dev2 = DEVICE_DT_GET(DT_CHOSEN(zephyr_shell_uart));

    // (Re)initialize the shell
    bool log_backend = true;
    uint32_t level = 4U;
    ret = shell_init(sh, dev2, sh->ctx->cfg.flags, log_backend, level);
    if (ret < 0) {
        LogS("Shell init failed: %d\n", ret);
        return ret;
    }

    k_sleep(K_MSEC(10));

    // Start the shell
    ret = shell_start(sh);
    if (ret < 0) {
        LogS("Shell start failed: %d\n", ret);
        return ret;
    }

    return 0;
}

static bool shellUninitialized = false;

static void shell_uninit_cb(const struct shell *sh, int res) {
    shellUninitialized = true;
}

void UninitShell(void)
{
    int err;
    const struct shell *sh = NULL;

    sh = shell_backend_uart_get_ptr();
    shellUninitialized = false;

	shell_uninit(sh, shell_uninit_cb);

    while (!shellUninitialized) {
        k_sleep(K_MSEC(10));
    }
}

void ReinitShell(void) {
    if (!DEVICE_IS_UHK80_RIGHT) {
        return;
    }

    if (PinWiringConfig->device_uart_shell == NULL) {
        return;
    } else {
        reinitShell(PinWiringConfig->device_uart_shell->device);
    }
}

void InitShellCommands(void)
{

    SHELL_STATIC_SUBCMD_SET_CREATE(uhk_cmds,
        SHELL_CMD_ARG(keylog, NULL, "get/set key logging", cmd_uhk_keylog, 1, 1),
#if !DEVICE_IS_UHK_DONGLE
        SHELL_CMD_ARG(statlog, NULL, "get/set stat logging", cmd_uhk_statlog, 1, 1),
        SHELL_CMD_ARG(leds, NULL, "get/set LEDs always on state", cmd_uhk_leds, 1, 1),
        SHELL_CMD_ARG(sdb, NULL, "get/set LED driver SDB pin", cmd_uhk_sdb, 1, 1),
        SHELL_CMD_ARG(charger, NULL, "get/set CHARGER_EN pin", cmd_uhk_charger, 1, 1),
#endif // !DEVICE_IS_UHK_DONGLE
#if DEVICE_IS_UHK80_RIGHT
        SHELL_CMD_ARG(testled, NULL, "enable led test mode", cmd_uhk_testled, 0, 1),
        SHELL_CMD_ARG(ledtest, NULL, "enable led test mode", cmd_uhk_testled, 0, 1),
#endif
#if DEVICE_HAS_OLED
        SHELL_CMD_ARG(oled, NULL, "get/set OLED_EN pin", cmd_uhk_oled, 1, 1),
#endif
#if DEVICE_HAS_MERGE_SENSOR
        SHELL_CMD_ARG(merge, NULL, "get the merged state of UHK halves", cmd_uhk_merge, 1, 0),
#endif
        SHELL_CMD_ARG(
            rollover, NULL, "get/set keyboard rollover mode (n/6)", cmd_uhk_rollover, 1, 1),
        SHELL_CMD_ARG(gamepad, NULL, "switch gamepad on/off", cmd_uhk_gamepad, 1, 1),
        SHELL_CMD_ARG(passkey, NULL, "send passkey for bluetooth pairing", cmd_uhk_passkey, 2, 0),
        SHELL_CMD_ARG(btunpair, NULL, "unpair bluetooth devices", cmd_uhk_btunpair, 1, 1),
        SHELL_CMD_ARG(connections, NULL, "list BLE connections", cmd_uhk_connections, 1, 0),
        SHELL_CMD_ARG(threads, NULL, "list thread statistics", cmd_uhk_threads, 1, 0),
        SHELL_CMD_ARG(trace, NULL, "lists minimalistic event trace", cmd_uhk_trace, 1, 0),
        SHELL_CMD_ARG(mouseMultipliers, NULL, "print mouse multipliers", cmd_uhk_mouseMultipliers, 1, 2),
        SHELL_CMD_ARG(logPriority, NULL, "set log priority", cmd_uhk_logPriority, 1, 2),
        SHELL_SUBCMD_SET_END);

    SHELL_CMD_REGISTER(uhk, &uhk_cmds, "UHK commands", NULL);
}

