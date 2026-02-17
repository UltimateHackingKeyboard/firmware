#include "sinks.h"
#include "wormhole.h"
#include "macros/status_buffer.h"
#include "trace.h"
#include "config_manager.h"

bool ShellConfig_IsInPanicMode = false;

bool ShellConfig_UseShellSinks = false;

static shell_sinks_t emptyConfig() {
    return (shell_sinks_t){
        .toUsbBuffer = false,
            .toOled = false,
            .toStatusBuffer = false,
    };
}

static shell_sinks_t activeConfig() {
    return (shell_sinks_t){
        .toUsbBuffer = WormCfg->UsbLogEnabled,
        .toOled = WormCfg->UsbLogEnabled,
        .toStatusBuffer = ShellConfig_IsInPanicMode,
    };
}



void ShellConfig_ActivatePanicMode(void)
{
    if (!ShellConfig_IsInPanicMode) {
        StateWormhole_Open();
        StateWormhole.persistStatusBuffer = true;

        ShellConfig_IsInPanicMode = true;

        MacroStatusBuffer_Validate();
        printk("===== PANIC =====\n");
        Trace_Print(LogTarget_ErrorBuffer, "crash/panic");
    }
}

shell_sinks_t ShellConfig_GetShellSinks(void)
{
    return ShellConfig_UseShellSinks ? activeConfig() : emptyConfig();
}

shell_sinks_t ShellConfig_GetLogSinks(void)
{
    return ShellConfig_UseShellSinks ? emptyConfig() : activeConfig();
}
