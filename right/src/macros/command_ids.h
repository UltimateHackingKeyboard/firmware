#ifndef __COMMAND_IDS_H__
#define __COMMAND_IDS_H__

// Command IDs for the gperf-generated hash table
typedef enum {
    CommandId_None = 0,

    // 'a' commands
    CommandId_activateKeyPostponed,
    CommandId_autoRepeat,
    CommandId_addReg,  // deprecated

    // 'b' commands
    CommandId_break,
    CommandId_bluetooth,

    // 'c' commands
    CommandId_consumePending,
    CommandId_clearStatus,
    CommandId_call,

    // 'd' commands
    CommandId_delayUntilRelease,
    CommandId_delayUntilReleaseMax,
    CommandId_delayUntil,
    CommandId_diagnose,

    // 'e' commands
    CommandId_exec,
    CommandId_else,
    CommandId_exit,

    // 'f' commands
    CommandId_final,
    CommandId_fork,
    CommandId_freeze,

    // 'g' commands
    CommandId_goTo,

    // 'h' commands
    CommandId_holdLayer,
    CommandId_holdLayerMax,
    CommandId_holdKeymapLayer,
    CommandId_holdKeymapLayerMax,
    CommandId_holdKey,

    // 'i' commands - conditionals
    CommandId_if,
    CommandId_ifDoubletap,
    CommandId_ifNotDoubletap,
    CommandId_ifInterrupted,
    CommandId_ifNotInterrupted,
    CommandId_ifReleased,
    CommandId_ifNotReleased,
    CommandId_ifKeymap,
    CommandId_ifNotKeymap,
    CommandId_ifLayer,
    CommandId_ifNotLayer,
    CommandId_ifLayerToggled,
    CommandId_ifNotLayerToggled,
    CommandId_ifPlaytime,
    CommandId_ifNotPlaytime,
    CommandId_ifAnyMod,
    CommandId_ifNotAnyMod,
    CommandId_ifShift,
    CommandId_ifNotShift,
    CommandId_ifCtrl,
    CommandId_ifNotCtrl,
    CommandId_ifAlt,
    CommandId_ifNotAlt,
    CommandId_ifGui,
    CommandId_ifNotGui,
    CommandId_ifCapsLockOn,
    CommandId_ifNotCapsLockOn,
    CommandId_ifNumLockOn,
    CommandId_ifNotNumLockOn,
    CommandId_ifScrollLockOn,
    CommandId_ifNotScrollLockOn,
    CommandId_ifRecording,
    CommandId_ifNotRecording,
    CommandId_ifRecordingId,
    CommandId_ifNotRecordingId,
    CommandId_ifNotPending,
    CommandId_ifPending,
    CommandId_ifKeyPendingAt,
    CommandId_ifNotKeyPendingAt,
    CommandId_ifKeyActive,
    CommandId_ifNotKeyActive,
    CommandId_ifPendingKeyReleased,
    CommandId_ifNotPendingKeyReleased,
    CommandId_ifKeyDefined,
    CommandId_ifNotKeyDefined,
    CommandId_ifModuleConnected,
    CommandId_ifNotModuleConnected,
    CommandId_ifHold,
    CommandId_ifTap,
    CommandId_ifSecondary,
    CommandId_ifPrimary,
    CommandId_ifShortcut,
    CommandId_ifNotShortcut,
    CommandId_ifGesture,
    CommandId_ifNotGesture,
    CommandId_ifRegEq,      // deprecated
    CommandId_ifNotRegEq,   // deprecated
    CommandId_ifRegGt,      // deprecated
    CommandId_ifRegLt,      // deprecated

    // 'm' commands
    CommandId_mulReg,  // deprecated

    // 'n' commands
    CommandId_noOp,
    CommandId_notify,

    // 'o' commands
    CommandId_oneShot,
    CommandId_overlayLayer,
    CommandId_overlayKeymap,

    // 'p' commands
    CommandId_printStatus,
    CommandId_playMacro,
    CommandId_pressKey,
    CommandId_postponeKeys,
    CommandId_postponeNext,
    CommandId_progressHue,
    CommandId_powerMode,
    CommandId_panic,

    // 'r' commands
    CommandId_recordMacro,
    CommandId_recordMacroBlind,
    CommandId_recordMacroDelay,
    CommandId_resolveNextKeyId,
    CommandId_releaseKey,
    CommandId_repeatFor,
    CommandId_resetTrackpoint,
    CommandId_replaceLayer,
    CommandId_replaceKeymap,
    CommandId_resolveNextKeyEq,  // deprecated
    CommandId_resolveSecondary,  // deprecated
    CommandId_resetConfiguration,
    CommandId_reboot,
    CommandId_reconnect,

    // 's' commands
    CommandId_set,
    CommandId_setVar,
    CommandId_setStatus,
    CommandId_startRecording,
    CommandId_startRecordingBlind,
    CommandId_setLedTxt,
    CommandId_statsRuntime,
    CommandId_statsRecordKeyTiming,
    CommandId_statsLayerStack,
    CommandId_statsActiveKeys,
    CommandId_statsActiveMacros,
    CommandId_statsPostponerStack,
    CommandId_statsVariables,
    CommandId_statsBattery,
    CommandId_switchKeymap,
    CommandId_startMouse,
    CommandId_stopMouse,
    CommandId_stopRecording,
    CommandId_stopAllMacros,
    CommandId_stopRecordingBlind,
    CommandId_suppressMods,
    CommandId_setReg,           // deprecated
    CommandId_subReg,           // deprecated
    CommandId_setStatusPart,    // deprecated
    CommandId_switchKeymapLayer, // deprecated
    CommandId_switchLayer,       // deprecated
    CommandId_switchHost,

    // 't' commands
    CommandId_toggleKeymapLayer,
    CommandId_toggleLayer,
    CommandId_tapKey,
    CommandId_tapKeySeq,
    CommandId_toggleKey,
    CommandId_trackpoint,
    CommandId_trace,
    CommandId_testLeakage,
    CommandId_testSuite,

    // 'u' commands
    CommandId_unToggleLayer,
    CommandId_untoggleLayer,
    CommandId_unpairHost,

    // 'v' commands
    CommandId_validateUserConfig,
    CommandId_validateMacros,

    // 'w' commands
    CommandId_write,
    CommandId_while,
    CommandId_writeExpr,  // deprecated

    // 'y' commands
    CommandId_yield,

    // 'z' commands
    CommandId_zephyr,

    // brace commands
    CommandId_openBrace,
    CommandId_closeBrace,

    CommandId_Count
} command_id_t;

#endif
