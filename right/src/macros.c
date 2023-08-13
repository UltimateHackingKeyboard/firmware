#include "macros.h"
#include "layer_stack.h"
#include <stdarg.h>
#include "event_scheduler.h"
#include <math.h>
#include "layer.h"
#include "secondary_role_driver.h"
#include "segment_display.h"
#include "usb_interfaces/usb_interface_basic_keyboard.h"
#include "usb_interfaces/usb_interface_media_keyboard.h"
#include "usb_interfaces/usb_interface_mouse.h"
#include "usb_interfaces/usb_interface_system_keyboard.h"
#include "config_parser/parse_macro.h"
#include "config_parser/config_globals.h"
#include "timer.h"
#include "usb_interfaces/usb_interface_basic_keyboard.h"
#include "keymap.h"
#include "usb_report_updater.h"
#include "led_display.h"
#include "postponer.h"
#include "ledmap.h"
#include "macro_recorder.h"
#include "macro_shortcut_parser.h"
#include "str_utils.h"
#include "utils.h"
#include "layer_switcher.h"
#include "mouse_controller.h"
#include "debug.h"
#include "macro_set_command.h"
#include "slave_drivers/uhk_module_driver.h"
#include <stddef.h>
#include <string.h>
#include "usb_commands/usb_command_exec_macro_command.h"

macro_reference_t AllMacros[MacroIndex_MaxCount] = {
    // 254 is reserved for USB command execution
    // 255 is reserved as empty value
    [MacroIndex_UsbCmdReserved] = {
        .macroActionsCount = 1,
    }
};
uint8_t AllMacrosCount;

uint8_t MacroBasicScancodeIndex = 0;
uint8_t MacroMediaScancodeIndex = 0;
uint8_t MacroSystemScancodeIndex = 0;

bool MacroPlaying = false;
bool Macros_WakedBecauseOfTime = false;
bool Macros_WakedBecauseOfKeystateChange = false;
uint32_t Macros_WakeMeOnTime = 0xFFFFFFFF;
bool Macros_WakeMeOnKeystateChange = false;

bool Macros_ParserError = false;

uint8_t consumeStatusCharReadingPos = 0;

macro_scheduler_t Macros_Scheduler = Scheduler_Blocking;
uint8_t Macros_MaxBatchSize = 20;
static scheduler_state_t scheduler = {
    .previousSlotIdx = 0,
    .currentSlotIdx = 0,
    .lastQueuedSlot = 0,
    .activeSlotCount = 0,
    .remainingCount = 0,
};

static char statusBuffer[STATUS_BUFFER_MAX_LENGTH];
static uint16_t statusBufferLen;
static bool statusBufferPrinting;

static uint8_t lastLayerIdx;
static uint8_t lastLayerKeymapIdx;
static uint8_t lastKeymapIdx;

static int32_t regs[MAX_REG_COUNT];

macro_state_t MacroState[MACRO_STATE_POOL_SIZE];
static macro_state_t *s = NULL;

uint16_t DoubletapConditionTimeout = 400;
uint16_t AutoRepeatInitialDelay = 500;
uint16_t AutoRepeatDelayRate = 50;

static void checkSchedulerHealth(const char* tag);
static void wakeMacroInSlot(uint8_t slotIdx);
static void scheduleSlot(uint8_t slotIdx);
static void unscheduleCurrentSlot();
static int32_t parseNUM(const char *a, const char *aEnd);
static macro_result_t processCommand(const char* cmd, const char* cmdEnd);
static macro_result_t processCommandAction(void);
static macro_result_t continueMacro(void);
static macro_result_t execMacro(uint8_t macroIndex);
static macro_result_t callMacro(uint8_t macroIndex);
static macro_result_t forkMacro(uint8_t macroIndex);
static bool loadNextCommand();
static bool loadNextAction();
static void resetToAddressZero(uint8_t macroIndex);
static uint8_t currentActionCmdCount();
static macro_result_t sleepTillTime(uint32_t time);
static macro_result_t sleepTillKeystateChange();


void Macros_SignalInterrupt()
{
    for (uint8_t i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
        if (MacroState[i].ms.macroPlaying) {
            MacroState[i].ms.macroInterrupted = true;
        }
    }
}

static void addBasicScancode(uint8_t scancode)
{
    if (!scancode) {
        return;
    }
    if (!UsbBasicKeyboard_ContainsScancode(&s->ms.macroBasicKeyboardReport, scancode)) {
        UsbBasicKeyboard_AddScancode(&s->ms.macroBasicKeyboardReport, scancode);
    }
}

static void deleteBasicScancode(uint8_t scancode)
{
    if (!scancode) {
        return;
    }
    UsbBasicKeyboard_RemoveScancode(&s->ms.macroBasicKeyboardReport, scancode);
}

static void addModifiers(uint8_t inputModifiers, uint8_t outputModifiers)
{
    s->ms.inputModifierMask |= inputModifiers;
    s->ms.macroBasicKeyboardReport.modifiers |= outputModifiers;
}

static void deleteModifiers(uint8_t inputModifiers, uint8_t outputModifiers)
{
    s->ms.inputModifierMask &= ~inputModifiers;
    s->ms.macroBasicKeyboardReport.modifiers &= ~outputModifiers;
}

static void addMediaScancode(uint16_t scancode)
{
    if (!scancode) {
        return;
    }
    for (uint8_t i = 0; i < USB_MEDIA_KEYBOARD_MAX_KEYS; i++) {
        if (s->ms.macroMediaKeyboardReport.scancodes[i] == scancode) {
            return;
        }
    }
    for (uint8_t i = 0; i < USB_MEDIA_KEYBOARD_MAX_KEYS; i++) {
        if (!s->ms.macroMediaKeyboardReport.scancodes[i]) {
            s->ms.macroMediaKeyboardReport.scancodes[i] = scancode;
            break;
        }
    }
}

static void deleteMediaScancode(uint16_t scancode)
{
    if (!scancode) {
        return;
    }
    for (uint8_t i = 0; i < USB_MEDIA_KEYBOARD_MAX_KEYS; i++) {
        if (s->ms.macroMediaKeyboardReport.scancodes[i] == scancode) {
            s->ms.macroMediaKeyboardReport.scancodes[i] = 0;
            return;
        }
    }
}

static void addSystemScancode(uint8_t scancode)
{
    if (!scancode) {
        return;
    }
    UsbSystemKeyboard_AddScancode(&s->ms.macroSystemKeyboardReport, scancode);
}

static void deleteSystemScancode(uint8_t scancode)
{
    if (!scancode) {
        return;
    }
    UsbSystemKeyboard_RemoveScancode(&s->ms.macroSystemKeyboardReport, scancode);
}

static void addScancode(uint16_t scancode, keystroke_type_t type)
{
    switch (type) {
        case KeystrokeType_Basic:
            addBasicScancode(scancode);
            break;
        case KeystrokeType_Media:
            addMediaScancode(scancode);
            break;
        case KeystrokeType_System:
            addSystemScancode(scancode);
            break;
    }
}

static void deleteScancode(uint16_t scancode, keystroke_type_t type)
{
    switch (type) {
        case KeystrokeType_Basic:
            deleteBasicScancode(scancode);
            break;
        case KeystrokeType_Media:
            deleteMediaScancode(scancode);
            break;
        case KeystrokeType_System:
            deleteSystemScancode(scancode);
            break;
    }
}

static macro_result_t processDelay(uint32_t time)
{
    if (s->as.actionActive) {
        if (Timer_GetElapsedTime(&s->as.delayData.start) >= time) {
            s->as.actionActive = false;
            s->as.delayData.start = 0;
            return MacroResult_Finished;
        }
        sleepTillTime(s->as.delayData.start + time);
        return MacroResult_Sleeping;
    } else {
        s->as.delayData.start = CurrentTime;
        s->as.actionActive = true;
        return processDelay(time);
    }
}

static macro_result_t processDelayAction()
{
    return processDelay(s->ms.currentMacroAction.delay.delay);
}


static void postponeNextN(uint8_t count)
{
    s->ms.postponeNextNCommands = count + 1;
    s->as.modifierPostpone = true;
    PostponerCore_PostponeNCycles(MACRO_CYCLES_TO_POSTPONE);
}

static void postponeCurrentCycle()
{
    PostponerCore_PostponeNCycles(MACRO_CYCLES_TO_POSTPONE);
    s->as.modifierPostpone = true;
}

/**
 * Both key press and release are subject to postponing, therefore we need to ensure
 * that macros which actively initiate postponing and wait until release ignore
 * postponed key releases. The s->postponeNext indicates that the running macro
 * initiates postponing in the current cycle.
 */
static bool currentMacroKeyIsActive()
{
    if (s->ms.currentMacroKey == NULL) {
        return s->ms.oneShotState;
    }
    if (s->ms.postponeNextNCommands > 0 || s->as.modifierPostpone) {
        bool keyIsActive = (KeyState_Active(s->ms.currentMacroKey) && !PostponerQuery_IsKeyReleased(s->ms.currentMacroKey));
        return  keyIsActive || s->ms.oneShotState;
    } else {
        bool keyIsActive = KeyState_Active(s->ms.currentMacroKey);
        return keyIsActive || s->ms.oneShotState;
    }
}


static macro_result_t processKey(macro_action_t macro_action)
{
    s->ms.reportsUsed = true;
    macro_sub_action_t action = macro_action.key.action;
    keystroke_type_t type = macro_action.key.type;
    uint8_t inputModMask = macro_action.key.inputModMask;
    uint8_t outputModMask = macro_action.key.outputModMask;
    uint8_t stickyModMask = macro_action.key.stickyModMask;
    uint16_t scancode = macro_action.key.scancode;

    s->as.actionPhase++;

    switch (action) {
        case MacroSubAction_Hold:
        case MacroSubAction_Tap:
            switch(s->as.actionPhase) {
                case 1:
                    addModifiers(inputModMask, outputModMask);
                    if (stickyModMask) {
                        ActivateStickyMods(s->ms.currentMacroKey, stickyModMask);
                    }
                    return MacroResult_Blocking;
                case 2:
                    addScancode(scancode, type);
                    return MacroResult_Blocking;
                case 3:
                    if (currentMacroKeyIsActive() && action == MacroSubAction_Hold) {
                        s->as.actionPhase--;
                        return sleepTillKeystateChange();
                    }
                    deleteScancode(scancode, type);
                    return MacroResult_Blocking;
                case 4:
                    deleteModifiers(inputModMask, outputModMask);
                    return MacroResult_Blocking;
                case 5:
                    s->as.actionPhase = 0;
                    return MacroResult_Finished;
            }
            break;
        case MacroSubAction_Release:
            switch (s->as.actionPhase) {
                case 1:
                    deleteScancode(scancode, type);
                    return MacroResult_Blocking;
                case 2:
                    deleteModifiers(inputModMask, outputModMask);
                    return MacroResult_Blocking;
                case 3:
                    s->as.actionPhase = 0;
                    return MacroResult_Finished;
            }
            break;
        case MacroSubAction_Press:
            switch (s->as.actionPhase) {
                case 1:
                    addModifiers(inputModMask, outputModMask);
                    if (stickyModMask) {
                        ActivateStickyMods(s->ms.currentMacroKey, stickyModMask);
                    }
                    return MacroResult_Blocking;
                case 2:
                    addScancode(scancode, type);
                    return MacroResult_Blocking;
                case 3:
                    s->as.actionPhase = 0;
                    return MacroResult_Finished;
            }
            break;
    }
    return MacroResult_Finished;
}

static macro_result_t processKeyAction()
{
    return processKey(s->ms.currentMacroAction);
}

static macro_result_t processMouseButton(macro_action_t macro_action)
{
    s->ms.reportsUsed = true;
    uint8_t mouseButtonMask = macro_action.mouseButton.mouseButtonsMask;
    macro_sub_action_t action = macro_action.mouseButton.action;

    s->as.actionPhase++;

    switch (macro_action.mouseButton.action) {
        case MacroSubAction_Hold:
        case MacroSubAction_Tap:
            switch(s->as.actionPhase) {
            case 1:
                s->ms.macroMouseReport.buttons |= mouseButtonMask;
                return MacroResult_Blocking;
            case 2:
                if (currentMacroKeyIsActive() && action == MacroSubAction_Hold) {
                    s->as.actionPhase--;
                    return sleepTillKeystateChange();
                }
                s->ms.macroMouseReport.buttons &= ~mouseButtonMask;
                return MacroResult_Blocking;
            case 3:
                s->as.actionPhase = 0;
                return MacroResult_Finished;

            }
            break;
        case MacroSubAction_Release:
            switch(s->as.actionPhase) {
            case 1:
                s->ms.macroMouseReport.buttons &= ~mouseButtonMask;
                return MacroResult_Blocking;
            case 2:
                s->as.actionPhase = 0;
                return MacroResult_Finished;
            }
            break;
        case MacroSubAction_Press:
            switch(s->as.actionPhase) {
                case 1:
                    s->ms.macroMouseReport.buttons |= mouseButtonMask;
                    return MacroResult_Blocking;
                case 2:
                    s->as.actionPhase = 0;
                    return MacroResult_Finished;
            }
            break;
    }
    return MacroResult_Finished;
}

static macro_result_t processMouseButtonAction(void)
{
    return processMouseButton(s->ms.currentMacroAction);
}

static macro_result_t processMoveMouseAction(void)
{
    s->ms.reportsUsed = true;
    if (s->as.actionActive) {
        s->ms.macroMouseReport.x = 0;
        s->ms.macroMouseReport.y = 0;
        s->as.actionActive = false;
    } else {
        s->ms.macroMouseReport.x = s->ms.currentMacroAction.moveMouse.x;
        s->ms.macroMouseReport.y = s->ms.currentMacroAction.moveMouse.y;
        s->as.actionActive = true;
    }
    return s->as.actionActive ? MacroResult_Blocking : MacroResult_Finished;
}

static macro_result_t processScrollMouseAction(void)
{
    s->ms.reportsUsed = true;
    if (s->as.actionActive) {
        s->ms.macroMouseReport.wheelX = 0;
        s->ms.macroMouseReport.wheelY = 0;
        s->as.actionActive = false;
    } else {
        s->ms.macroMouseReport.wheelX = s->ms.currentMacroAction.scrollMouse.x;
        s->ms.macroMouseReport.wheelY = s->ms.currentMacroAction.scrollMouse.y;
        s->as.actionActive = true;
    }
    return s->as.actionActive ? MacroResult_Blocking : MacroResult_Finished;
}

static macro_result_t processClearStatusCommand()
{
    statusBufferLen = 0;
    consumeStatusCharReadingPos = 0;
    SegmentDisplay_DeactivateSlot(SegmentDisplaySlot_Error);
    SegmentDisplay_DeactivateSlot(SegmentDisplaySlot_Warn);
    return MacroResult_Finished;
}


char Macros_ConsumeStatusChar()
{
    char res;

    if (consumeStatusCharReadingPos < statusBufferLen) {
        res = statusBuffer[consumeStatusCharReadingPos++];
    } else {
        res = '\0';
    }

    return res;
}

//textEnd is allowed to be null if text is null-terminated
static void setStatusStringInterpolated(const char* text, const char *textEnd, bool interpolated)
{
    if (statusBufferPrinting) {
        return;
    }
    while(*text && statusBufferLen < STATUS_BUFFER_MAX_LENGTH && (text < textEnd || textEnd == NULL)) {
        if (*text == '#' && interpolated) {
            int32_t parsed = Macros_ParseInt(text, textEnd, &text);
            //text should be now set to next character after a number, even if it was e.g., "'#3'"
            Macros_SetStatusNumSpaced(parsed, false);
        } else {
            statusBuffer[statusBufferLen] = *text;
            text++;
            statusBufferLen++;
        }
    }
}

void Macros_SetStatusString(const char* text, const char *textEnd)
{
    setStatusStringInterpolated(text, textEnd, false);
}

void Macros_SetStatusStringInterpolated(const char* text, const char *textEnd)
{
    setStatusStringInterpolated(text, textEnd, true);
}

void Macros_SetStatusBool(bool b)
{
    Macros_SetStatusString(b ? "1" : "0", NULL);
}

void Macros_SetStatusNumSpaced(int32_t n, bool spaced)
{
    char buff[2];
    buff[0] = ' ';
    buff[1] = '\0';
    if (spaced) {
        Macros_SetStatusString(" ", NULL);
    }
    if (n < 0) {
        n = -n;
        buff[0] = '-';
        Macros_SetStatusString(buff, NULL);
    }
    int32_t orig = n;
    for (uint32_t div = 1000000000; div > 0; div /= 10) {
        buff[0] = (char)(((uint8_t)(n/div)) + '0');
        n = n%div;
        if (n!=orig || div == 1) {
          Macros_SetStatusString(buff, NULL);
        }
    }
}

void Macros_SetStatusFloat(float n)
{
    float intPart = 0;
    float fraPart = modff(n, &intPart);
    Macros_SetStatusNumSpaced(intPart, true);
    Macros_SetStatusString(".", NULL);
    Macros_SetStatusNumSpaced((int32_t)(fraPart * 1000 / 1), false);
}

void Macros_SetStatusNum(int32_t n)
{
    Macros_SetStatusNumSpaced(n, true);
}

void Macros_SetStatusChar(char n)
{
    Macros_SetStatusString(&n, &n+1);
}

static void reportErrorHeader(const char* status)
{
    if (s != NULL) {
        const char *name, *nameEnd;
        FindMacroName(&AllMacros[s->ms.currentMacroIndex], &name, &nameEnd);
        Macros_SetStatusString(status, NULL);
        Macros_SetStatusString(" at ", NULL);
        Macros_SetStatusString(name, nameEnd);
        Macros_SetStatusString(" ", NULL);
        Macros_SetStatusNumSpaced(s->ms.currentMacroActionIndex, false);
        Macros_SetStatusString("/", NULL);
        Macros_SetStatusNumSpaced(s->ms.commandAddress, false);
        Macros_SetStatusString(": ", NULL);
    }
}


void Macros_ReportError(const char* err, const char* arg, const char *argEnd)
{
    Macros_ParserError = true;
    SegmentDisplay_SetText(3, "ERR", SegmentDisplaySlot_Error);
    reportErrorHeader("Error");
    Macros_SetStatusString(err, NULL);
    if (arg != NULL) {
        Macros_SetStatusString(": ", NULL);
        Macros_SetStatusString(arg, argEnd);
    }
    Macros_SetStatusString("\n", NULL);
}

void Macros_ReportWarn(const char* err, const char* arg, const char *argEnd)
{
    SegmentDisplay_SetText(3, "WRN", SegmentDisplaySlot_Warn);
    reportErrorHeader("Warning");
    Macros_SetStatusString(err, NULL);
    if (arg != NULL) {
        Macros_SetStatusString(": ", NULL);
        Macros_SetStatusString(arg, argEnd);
    }
    Macros_SetStatusString("\n", NULL);
}


void Macros_ReportErrorPrintf(const char *fmt, ...)
{
    va_list myargs;
    va_start(myargs, fmt);
    char buffer[256];
    vsprintf(buffer, fmt, myargs);
    Macros_ReportError(buffer, NULL, NULL);

}

void Macros_ReportErrorFloat(const char* err, float num)
{
    Macros_ParserError = true;
    SegmentDisplay_SetText(3, "ERR", SegmentDisplaySlot_Error);
    reportErrorHeader("Error");
    Macros_SetStatusString(err, NULL);
    Macros_SetStatusFloat(num);
    Macros_SetStatusString("\n", NULL);
}

void Macros_ReportErrorNum(const char* err, int32_t num)
{
    Macros_ParserError = true;
    SegmentDisplay_SetText(3, "ERR", SegmentDisplaySlot_Error);
    reportErrorHeader("Error");
    Macros_SetStatusString(err, NULL);
    Macros_SetStatusNum(num);
    Macros_SetStatusString("\n", NULL);
}

static void clearScancodes()
{
    uint8_t oldMods = s->ms.macroBasicKeyboardReport.modifiers;
    memset(&s->ms.macroBasicKeyboardReport, 0, sizeof s->ms.macroBasicKeyboardReport);
    s->ms.macroBasicKeyboardReport.modifiers = oldMods;
}

static macro_result_t dispatchText(const char* text, uint16_t textLen)
{
    s->ms.reportsUsed = true;
    static macro_state_t* dispatchMutex = NULL;
    if (dispatchMutex != s && dispatchMutex != NULL) {
        return MacroResult_Waiting;
    } else {
        dispatchMutex = s;
    }
    char character = 0;
    uint8_t scancode = 0;
    uint8_t mods = 0;

    // Precompute modifiers and scancode.
    if (s->as.dispatchData.textIdx != textLen) {
        character = text[s->as.dispatchData.textIdx];
        scancode = MacroShortcutParser_CharacterToScancode(character);
        mods = MacroShortcutParser_CharacterToShift(character) ? HID_KEYBOARD_MODIFIER_LEFTSHIFT : 0;
    }

    // If required modifiers differ, first clear scancodes and send empty report
    // containing only old modifiers. Then set new modifiers and send that new report.
    // Just then continue.
    if (mods != s->ms.macroBasicKeyboardReport.modifiers) {
        if (s->as.dispatchData.reportState != REPORT_EMPTY) {
            s->as.dispatchData.reportState = REPORT_EMPTY;
            clearScancodes();
            return MacroResult_Blocking;
        } else {
            s->ms.macroBasicKeyboardReport.modifiers = mods;
            return MacroResult_Blocking;
        }
    }

    // If all characters have been sent, finish.
    if (s->as.dispatchData.textIdx == textLen) {
        if (s->as.dispatchData.reportState != REPORT_EMPTY) {
            s->as.dispatchData.reportState = REPORT_EMPTY;
            memset(&s->ms.macroBasicKeyboardReport, 0, sizeof s->ms.macroBasicKeyboardReport);
            return MacroResult_Blocking;
        } else {
            s->as.dispatchData.textIdx = 0;
            dispatchMutex = NULL;
            return MacroResult_Finished;
        }
    }

    // Whenever the report is full, we clear the report and send it empty before continuing.
    if (s->as.dispatchData.reportState == REPORT_FULL) {
        s->as.dispatchData.reportState = REPORT_EMPTY;
        memset(&s->ms.macroBasicKeyboardReport, 0, sizeof s->ms.macroBasicKeyboardReport);
        return MacroResult_Blocking;
    }

    // If current character is already contained in the report, we need to
    // release it first. We do so by artificially marking the report
    // full. Next call will do rest of the work for us.
    if (UsbBasicKeyboard_ContainsScancode(&s->ms.macroBasicKeyboardReport, scancode)) {
        s->as.dispatchData.reportState = REPORT_FULL;
        return MacroResult_Blocking;
    }

    // Send the scancode.
    UsbBasicKeyboard_AddScancode(&s->ms.macroBasicKeyboardReport, scancode);
    s->as.dispatchData.reportState = UsbBasicKeyboard_IsFullScancodes(&s->ms.macroBasicKeyboardReport) ?
            REPORT_FULL : REPORT_PARTIAL;
    ++s->as.dispatchData.textIdx;
    return MacroResult_Blocking;
}

static macro_result_t processTextAction(void)
{
    return dispatchText(s->ms.currentMacroAction.text.text, s->ms.currentMacroAction.text.textLen);
}

static bool validReg(uint8_t idx)
{
    if (idx >= MAX_REG_COUNT) {
        Macros_ReportErrorNum("Invalid register index:", idx);
        return false;
    }
    return true;
}

static macro_result_t writeNum(uint32_t a)
{
    char num[11];
    num[10] = '\0';
    int at = 9;
    while ((a > 0 || at == 9) && at >= 0) {
        num[at] = a % 10 + 48;
        a = a/10;
        at--;
    }

    macro_result_t res = dispatchText(&num[at+1], 9 - at);
    if (res == MacroResult_Finished) {
        PostponerExtended_ConsumePendingKeypresses(1, true);
        return MacroResult_Finished;
    }
    return res;
}

static bool isNUM(const char *a, const char *aEnd)
{
    switch(*a) {
    case '0'...'9':
    case '#':
    case '-':
    case '@':
    case '%':
        return true;
    default:
        return false;
    }
}

int32_t Macros_ParseInt(const char *a, const char *aEnd, const char* *parsedTill)
{
    if (*a == '#') {
        a++;
        if (TokenMatches(a, aEnd, "key")) {
            if (parsedTill != NULL) {
                *parsedTill = a+3;
            }
            return Utils_KeyStateToKeyId(s->ms.currentMacroKey);
        }
        uint8_t adr = Macros_ParseInt(a, aEnd, parsedTill);
        if (validReg(adr)) {
            return regs[adr];
        } else {
            return 0;
        }
    }
    else if (*a == '%') {
        a++;
        uint8_t idx = Macros_ParseInt(a, aEnd, parsedTill);
        if (idx >= PostponerQuery_PendingKeypressCount()) {
            Macros_ReportError("Not enough pending keys! Note that this is zero-indexed!",  NULL,  NULL);
            return 0;
        }
        return PostponerExtended_PendingId(idx);
    }
    else if (*a == '@') {
        a++;
        return s->ms.commandAddress + Macros_ParseInt(a, aEnd, parsedTill);
    }
    else
    {
        return ParseInt32_2(a, aEnd, parsedTill);
    }
}

bool Macros_ParseBoolean(const char *a, const char *aEnd)
{
    if (TokenMatches(a, aEnd, "1")) {
        return true;
    }
    else if (TokenMatches(a, aEnd, "0")) {
        return false;
    } else {
        Macros_ReportError("Boolean value expected, got:",  a, aEnd);
        return false;
    }
}

static int32_t parseNUM(const char *a, const char *aEnd)
{
    return Macros_ParseInt(a, aEnd, NULL);
}

static int32_t parseRuntimeMacroSlotId(const char *a, const char *aEnd)
{
    const char* end = TokEnd(a, aEnd);
    static uint16_t lastMacroId = 0;
    if (TokenMatches(a, aEnd, "last")) {
        return lastMacroId;
    }
    else if (a == aEnd) {
        lastMacroId = Utils_KeyStateToKeyId(s->ms.currentMacroKey);
    } else if (end == a+1) {
        lastMacroId = (uint8_t)(*a);
    } else {
        lastMacroId = 128 + parseNUM(a, aEnd);
    }
    return lastMacroId;
}

static macro_result_t processStatsLayerStackCommand()
{
    Macros_SetStatusString("kmp/layer/held/removed; size is ", NULL);
    Macros_SetStatusNum(LayerStack_Size);
    Macros_SetStatusString("\n", NULL);
    for (int i = 0; i < LayerStack_Size; i++) {
        uint8_t pos = (LayerStack_TopIdx + LAYER_STACK_SIZE - i) % LAYER_STACK_SIZE;
        Macros_SetStatusNum(LayerStack[pos].keymap);
        Macros_SetStatusString("/", NULL);
        Macros_SetStatusNum(LayerStack[pos].layer);
        Macros_SetStatusString("/", NULL);
        Macros_SetStatusNum(LayerStack[pos].held);
        Macros_SetStatusString("/", NULL);
        Macros_SetStatusNum(LayerStack[pos].removed);
        Macros_SetStatusString("\n", NULL);
    }
    return MacroResult_Finished;
}

static macro_result_t processStatsActiveKeysCommand()
{
    Macros_SetStatusString("keyid/previous/current/debouncing\n", NULL);
    for (uint8_t slotId=0; slotId<SLOT_COUNT; slotId++) {
        for (uint8_t keyId=0; keyId<MAX_KEY_COUNT_PER_MODULE; keyId++) {
            key_state_t *keyState = &KeyStates[slotId][keyId];
            if (keyState->current || keyState->previous) {
                Macros_SetStatusNum(Utils_KeyStateToKeyId(keyState));
                Macros_SetStatusString("/", NULL);
                Macros_SetStatusNum(keyState->previous);
                Macros_SetStatusString("/", NULL);
                Macros_SetStatusNum(keyState->current);
                Macros_SetStatusString("/", NULL);
                Macros_SetStatusNum(keyState->debouncing);
                Macros_SetStatusString("/", NULL);
            }
        }
    }
    return MacroResult_Finished;
}

static macro_result_t processStatsPostponerStackCommand()
{
    PostponerExtended_PrintContent();
    return MacroResult_Finished;
}

static void describeSchedulerState()
{
    Macros_SetStatusString("s:", NULL);
    Macros_SetStatusNum(scheduler.currentSlotIdx);
    Macros_SetStatusNum(scheduler.previousSlotIdx);
    Macros_SetStatusNum(scheduler.lastQueuedSlot);
    Macros_SetStatusNum(scheduler.activeSlotCount);

    Macros_SetStatusString(":", NULL);
    uint8_t slot = scheduler.currentSlotIdx;
    for (int i = 0; i < scheduler.activeSlotCount; i++) {
        Macros_SetStatusNum(slot);
        Macros_SetStatusString(" ", NULL);
        slot = MacroState[slot].ms.nextSlot;
    }
    Macros_SetStatusString("\n", NULL);
}

static macro_result_t processStatsActiveMacrosCommand()
{
    Macros_SetStatusString("Macro playing: ", NULL);
    Macros_SetStatusNum(MacroPlaying);
    Macros_SetStatusString("\n", NULL);
    Macros_SetStatusString("macro/slot/adr/properties\n", NULL);
    for (int i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
        if (MacroState[i].ms.macroPlaying) {
            const char *name, *nameEnd;
            FindMacroName(&AllMacros[MacroState[i].ms.currentMacroIndex], &name, &nameEnd);
            Macros_SetStatusString(" ", NULL);
            Macros_SetStatusString(name, nameEnd);
            Macros_SetStatusString("/", NULL);
            Macros_SetStatusNum((&MacroState[i]) - MacroState);
            Macros_SetStatusString("/", NULL);
            Macros_SetStatusNum(MacroState[i].ms.currentMacroActionIndex);
            Macros_SetStatusString("/", NULL);
            if (MacroState[i].as.modifierPostpone) {
                Macros_SetStatusString("mp ", NULL);
            }
            if (MacroState[i].as.modifierSuppressMods) {
                Macros_SetStatusString("ms ", NULL);
            }
            if (MacroState[i].ms.macroSleeping) {
                Macros_SetStatusString("s ", NULL);
            }
            if (MacroState[i].ms.wakeMeOnKeystateChange) {
                Macros_SetStatusString("ws ", NULL);
            }
            if (MacroState[i].ms.wakeMeOnTime) {
                Macros_SetStatusString("wt ", NULL);
            }
            Macros_SetStatusString("\n", NULL);

        }
    }
    describeSchedulerState();
    return MacroResult_Finished;
}

static macro_result_t processStatsRegs()
{
    Macros_SetStatusString("reg/val\n", NULL);
    for (int i = 0; i < MAX_REG_COUNT; i++) {
        Macros_SetStatusNum(i);
        Macros_SetStatusString("/", NULL);
        Macros_SetStatusNum(regs[i]);
        Macros_SetStatusString("\n", NULL);
    }
    return MacroResult_Finished;
}

static macro_result_t stopAllMacrosCommand()
{
    for (uint8_t i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
        if (&MacroState[i] != s) {
            MacroState[i].ms.macroBroken = true;
            MacroState[i].ms.macroSleeping = false;
        }
    }
    return MacroResult_Finished;
}

static macro_result_t processDiagnoseCommand()
{
    processStatsLayerStackCommand();
    processStatsActiveKeysCommand();
    processStatsPostponerStackCommand();
    processStatsActiveMacrosCommand();
    for (uint8_t slotId=0; slotId<SLOT_COUNT; slotId++) {
        for (uint8_t keyId=0; keyId<MAX_KEY_COUNT_PER_MODULE; keyId++) {
            key_state_t *keyState = &KeyStates[slotId][keyId];
            if (keyState != s->ms.currentMacroKey) {
                keyState->current = 0;
                keyState->previous = 0;
            }
        }
    }
    PostponerExtended_ResetPostponer();
    return MacroResult_Finished;
}


static uint8_t parseKeymapId(const char* arg1, const char* cmdEnd)
{
    if (TokenMatches(arg1, cmdEnd, "last")) {
        return lastKeymapIdx;
    } else {
        uint8_t idx = FindKeymapByAbbreviation(TokLen(arg1, cmdEnd), arg1);
        if (idx == 0xFF) {
            Macros_ReportError("Keymap not recognized: ", arg1, cmdEnd);
        }
        return idx;
    }
}

uint8_t Macros_ParseLayerId(const char* arg1, const char* cmdEnd)
{
    switch(*arg1) {
        case 'a':
            if (TokenMatches(arg1, cmdEnd, "alt")) {
                return LayerId_Alt;
            }
            break;
        case 'b':
            if (TokenMatches(arg1, cmdEnd, "base")) {
                return LayerId_Base;
            }
            break;
        case 'c':
            if (TokenMatches(arg1, cmdEnd, "current")) {
                return ActiveLayer;
            } else if (TokenMatches(arg1, cmdEnd, "ctrl")) {
                return LayerId_Ctrl;
            } else if (TokenMatches(arg1, cmdEnd, "control")) {
                return LayerId_Ctrl;
            }
            break;
        case 'f':
            if (TokenMatches(arg1, cmdEnd, "fn")) {
                return LayerId_Fn;
            } else if (TokenMatches(arg1, cmdEnd, "fn2")) {
                return LayerId_Fn2;
            } else if (TokenMatches(arg1, cmdEnd, "fn3")) {
                return LayerId_Fn3;
            } else if (TokenMatches(arg1, cmdEnd, "fn4")) {
                return LayerId_Fn4;
            } else if (TokenMatches(arg1, cmdEnd, "fn5")) {
                return LayerId_Fn5;
            }
            break;
        case 'g':
            if (TokenMatches(arg1, cmdEnd, "gui")) {
                return LayerId_Gui;
            }
            break;
        case 'l':
            if (TokenMatches(arg1, cmdEnd, "last")) {
                return lastLayerIdx;
            }
            break;
        case 'm':
            if (TokenMatches(arg1, cmdEnd, "mouse")) {
                return LayerId_Mouse;
            } else if (TokenMatches(arg1, cmdEnd, "mod")) {
                return LayerId_Mod;
            }
            break;
        case 'p':
            if (TokenMatches(arg1, cmdEnd, "previous")) {
                return LayerStack[LayerStack_FindPreviousLayerRecordIdx()].layer;
            }
            break;
        case 's':
            if (TokenMatches(arg1, cmdEnd, "shift")) {
                return LayerId_Shift;
            } else if (TokenMatches(arg1, cmdEnd, "super")) {
                return LayerId_Gui;
            }
            break;
    }

    Macros_ReportError("Unrecognized layer.", arg1, cmdEnd);
    return LayerId_Base;
}


static uint8_t parseLayerKeymapId(const char* arg1, const char* cmdEnd)
{
    if (TokenMatches(arg1, cmdEnd, "last")) {
        return lastLayerKeymapIdx;
    }
    else if (TokenMatches(arg1, cmdEnd, "previous")) {
        return LayerStack[LayerStack_FindPreviousLayerRecordIdx()].keymap;
    }
    else {
        return CurrentKeymapIndex;
    }
}

static macro_result_t processSwitchKeymapCommand(const char* arg1, const char* cmdEnd)
{
    uint8_t tmpKeymapIdx = CurrentKeymapIndex;
    {
        uint8_t newKeymapIdx = parseKeymapId(arg1, cmdEnd);

        if (Macros_ParserError) {
            return MacroResult_Finished;
        }

        SwitchKeymapById(newKeymapIdx);
        LayerStack_Reset();
    }
    lastKeymapIdx = tmpKeymapIdx;
    return MacroResult_Finished;
}

/**DEPRECATED**/
static macro_result_t processSwitchKeymapLayerCommand(const char* arg1, const char* cmdEnd)
{
    Macros_ReportWarn("Command deprecated. Please, replace switchKeymapLayer by toggleKeymapLayer or holdKeymapLayer. Or complain on github that you actually need this command.", NULL, NULL);
    uint8_t tmpLayerIdx = LayerStack_ActiveLayer;
    uint8_t tmpLayerKeymapIdx = CurrentKeymapIndex;
    uint8_t layer = Macros_ParseLayerId(NextTok(arg1, cmdEnd), cmdEnd);
    uint8_t keymap = parseKeymapId(arg1, cmdEnd);

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }

    LayerStack_Push(layer, keymap, false);
    lastLayerIdx = tmpLayerIdx;
    lastLayerKeymapIdx = tmpLayerKeymapIdx;
    return MacroResult_Finished;
}

/**DEPRECATED**/
static macro_result_t processSwitchLayerCommand(const char* arg1, const char* cmdEnd)
{
    Macros_ReportWarn("Command deprecated. Please, replace switchLayer by toggleLayer or holdLayer. Or complain on github that you actually need this command.", NULL, NULL);
    uint8_t tmpLayerIdx = LayerStack_ActiveLayer;
    uint8_t tmpLayerKeymapIdx = CurrentKeymapIndex;
    if (TokenMatches(arg1, cmdEnd, "previous")) {
        LayerStack_Pop(true, false);
    }
    else {
        uint8_t layer = Macros_ParseLayerId(arg1, cmdEnd);
        uint8_t keymap = parseKeymapId(arg1, cmdEnd);

        if (Macros_ParserError) {
            return MacroResult_Finished;
        }

        LayerStack_Push(layer, keymap, false);
    }
    lastLayerIdx = tmpLayerIdx;
    lastLayerKeymapIdx = tmpLayerKeymapIdx;
    return MacroResult_Finished;
}


static macro_result_t processToggleKeymapLayerCommand(const char* arg1, const char* cmdEnd)
{
    uint8_t tmpLayerIdx = LayerStack_ActiveLayer;
    uint8_t tmpLayerKeymapIdx = CurrentKeymapIndex;
    uint8_t layer = Macros_ParseLayerId(NextTok(arg1, cmdEnd), cmdEnd);
    uint8_t keymap = parseKeymapId(arg1, cmdEnd);

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }

    LayerStack_Push(layer, keymap, false);
    lastLayerIdx = tmpLayerIdx;
    lastLayerKeymapIdx = tmpLayerKeymapIdx;
    return MacroResult_Finished;
}

static macro_result_t processToggleLayerCommand(const char* arg1, const char* cmdEnd)
{
    uint8_t tmpLayerIdx = LayerStack_ActiveLayer;
    uint8_t tmpLayerKeymapIdx = CurrentKeymapIndex;
    uint8_t layer = Macros_ParseLayerId(arg1, cmdEnd);
    uint8_t keymap = parseLayerKeymapId(arg1, cmdEnd);

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }

    LayerStack_Push(layer, keymap, false);
    lastLayerIdx = tmpLayerIdx;
    lastLayerKeymapIdx = tmpLayerKeymapIdx;
    return MacroResult_Finished;
}

static macro_result_t processUnToggleLayerCommand()
{
    uint8_t tmpLayerIdx = LayerStack_ActiveLayer;
    uint8_t tmpLayerKeymapIdx = CurrentKeymapIndex;
    LayerStack_Pop(true, true);
    lastLayerIdx = tmpLayerIdx;
    lastLayerKeymapIdx = tmpLayerKeymapIdx;
    return MacroResult_Finished;
}

static macro_result_t processHoldLayer(uint8_t layer, uint8_t keymap, uint16_t timeout)
{
    if (!s->as.actionActive) {
        s->as.actionActive = true;
        LayerStack_Push(layer, keymap, true);
        s->as.holdLayerData.layerStackIdx = LayerStack_TopIdx;
        return MacroResult_Waiting;
    }
    else {
        if (currentMacroKeyIsActive() && (Timer_GetElapsedTime(&s->ms.currentMacroStartTime) < timeout || s->ms.macroInterrupted)) {
            if (!s->ms.macroInterrupted) {
                sleepTillTime(s->ms.currentMacroStartTime + timeout);
            }
            sleepTillKeystateChange();
            return MacroResult_Sleeping;
        }
        else {
            s->as.actionActive = false;
            LayerStack_RemoveRecord(s->as.holdLayerData.layerStackIdx);
            LayerStack_Pop(false, false);
            return MacroResult_Finished;
        }
    }
}

static macro_result_t processHoldLayerCommand(const char* arg1, const char* cmdEnd)
{
    uint8_t layer = Macros_ParseLayerId(arg1, cmdEnd);
    uint8_t keymap = parseLayerKeymapId(arg1, cmdEnd);

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }

    return processHoldLayer(layer, keymap, 0xFFFF);
}

static macro_result_t processHoldLayerMaxCommand(const char* arg1, const char* cmdEnd)
{
    const char* arg2 = NextTok(arg1, cmdEnd);

    uint8_t layer = Macros_ParseLayerId(arg1, cmdEnd);
    uint8_t keymap = parseLayerKeymapId(arg1, cmdEnd);
    uint16_t timeout = parseNUM(arg2, cmdEnd);

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }

    return processHoldLayer(layer, keymap, timeout);
}

static macro_result_t processHoldKeymapLayerCommand(const char* arg1, const char* cmdEnd)
{
    const char* arg2 = NextTok(arg1, cmdEnd);
    uint8_t layer = Macros_ParseLayerId(arg2, cmdEnd);
    uint8_t keymap = parseKeymapId(arg1, cmdEnd);

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }

    return processHoldLayer(layer, keymap, 0xFFFF);
}

static macro_result_t processHoldKeymapLayerMaxCommand(const char* arg1, const char* cmdEnd)
{
    const char* arg2 = NextTok(arg1, cmdEnd);
    const char* arg3 = NextTok(arg2, cmdEnd);

    uint8_t layer = Macros_ParseLayerId(arg2, cmdEnd);
    uint8_t keymap = parseKeymapId(arg1, cmdEnd);
    uint16_t timeout = parseNUM(arg3, cmdEnd);

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }

    return processHoldLayer(layer, keymap, timeout);
}

static macro_result_t processDelayUntilReleaseMaxCommand(const char* arg1, const char* cmdEnd)
{
    uint32_t timeout = parseNUM(arg1, cmdEnd);

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }

    if (currentMacroKeyIsActive() && Timer_GetElapsedTime(&s->ms.currentMacroStartTime) < timeout) {
        sleepTillKeystateChange();
        sleepTillTime(s->ms.currentMacroStartTime + timeout);
        return MacroResult_Sleeping;
    }
    return MacroResult_Finished;
}

static macro_result_t processDelayUntilReleaseCommand()
{
    if (currentMacroKeyIsActive()) {
        return sleepTillKeystateChange();
    }
    return MacroResult_Finished;
}

static macro_result_t processDelayUntilCommand(const char* arg1, const char* cmdEnd)
{
    uint32_t time = parseNUM(arg1,  cmdEnd);

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }

    return processDelay(time);
}

static macro_result_t processRecordMacroDelayCommand()
{
    if (currentMacroKeyIsActive()) {
        return MacroResult_Waiting;
    }
    uint16_t delay = Timer_GetElapsedTime(&s->ms.currentMacroStartTime);
    MacroRecorder_RecordDelay(delay);
    return MacroResult_Finished;
}

static bool processIfDoubletapCommand(bool negate)
{
    bool doubletapFound = false;

    for (uint8_t i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
        if (s->ms.currentMacroStartTime - MacroState[i].ps.previousMacroStartTime <= DoubletapConditionTimeout && s->ms.currentMacroIndex == MacroState[i].ps.previousMacroIndex) {
            doubletapFound = true;
        }
        if (
            MacroState[i].ms.macroPlaying &&
            MacroState[i].ms.currentMacroStartTime < s->ms.currentMacroStartTime &&
            s->ms.currentMacroStartTime - MacroState[i].ms.currentMacroStartTime <= DoubletapConditionTimeout &&
            s->ms.currentMacroIndex == MacroState[i].ms.currentMacroIndex &&
            &MacroState[i] != s
        ) {
            doubletapFound = true;
        }
    }

    return doubletapFound != negate;
}

static bool processIfModifierCommand(bool negate, uint8_t modmask)
{
    return ((InputModifiersPrevious & modmask) > 0) != negate;
}

static bool processIfStateKeyCommand(bool negate, bool *state)
{
    return *state != negate;
}

static bool processIfRecordingCommand(bool negate)
{
    return MacroRecorder_IsRecording() != negate;
}

static bool processIfRecordingIdCommand(bool negate, const char* arg, const char *argEnd)
{
    uint16_t id = parseRuntimeMacroSlotId(arg, argEnd);
    bool res = MacroRecorder_RecordingId() == id;
    return res != negate;
}

static bool processIfPendingCommand(bool negate, const char* arg, const char *argEnd)
{
    uint32_t cnt = parseNUM(arg, argEnd);

    return (PostponerQuery_PendingKeypressCount() >= cnt) != negate;
}

static bool processIfPlaytimeCommand(bool negate, const char* arg, const char *argEnd)
{
    uint32_t timeout = parseNUM(arg, argEnd);
    uint32_t delay = Timer_GetElapsedTime(&s->ms.currentMacroStartTime);
    return (delay > timeout) != negate;
}

static bool processIfInterruptedCommand(bool negate)
{
   return s->ms.macroInterrupted != negate;
}

static bool processIfReleasedCommand(bool negate)
{
   return (!currentMacroKeyIsActive()) != negate;
}

static bool processIfRegEqCommand(bool negate, const char* arg1, const char *argEnd)
{
    uint8_t address = parseNUM(arg1, argEnd);
    int32_t param = parseNUM(NextTok(arg1, argEnd), argEnd);
    if (validReg(address)) {
        bool res = regs[address] == param;
        return res != negate;
    } else {
        return false;
    }
}

static bool processIfRegInequalityCommand(bool greaterThan, const char* arg1, const char *argEnd)
{
    uint8_t address = parseNUM(arg1, argEnd);
    int32_t param = parseNUM(NextTok(arg1, argEnd), argEnd);
    if (validReg(address)) {
        if (greaterThan) {
            return regs[address] > param;
        } else {
            return regs[address] < param;
        }
    } else {
        return false;
    }
}

static bool processIfKeymapCommand(bool negate, const char* arg1, const char *argEnd)
{
    uint8_t queryKeymapIdx = parseKeymapId(arg1, argEnd);
    return (queryKeymapIdx == CurrentKeymapIndex) != negate;
}

static bool processIfLayerCommand(bool negate, const char* arg1, const char *argEnd)
{
    uint8_t queryLayerIdx = Macros_ParseLayerId(arg1, argEnd);
    return (queryLayerIdx == LayerStack_ActiveLayer) != negate;
}

static macro_result_t processBreakCommand()
{
    s->ms.macroBroken = true;
    return MacroResult_Finished;
}

static macro_result_t processPrintStatusCommand()
{
    statusBufferPrinting = true;
    macro_result_t res = dispatchText(statusBuffer, statusBufferLen);
    if (res == MacroResult_Finished) {
        processClearStatusCommand();
        statusBufferPrinting = false;
    }
    return res;
}

static macro_result_t processSetStatusCommand(const char* arg, const char *argEnd, bool addEndline)
{
    Macros_SetStatusStringInterpolated(arg, argEnd);
    if (addEndline) {
        Macros_SetStatusString("\n", NULL);
    }
    return MacroResult_Finished;
}

static macro_result_t processSetLedTxtCommand(const char* arg1, const char *argEnd)
{
    int16_t time = parseNUM(arg1, argEnd);
    macro_result_t res = MacroResult_Finished;
    if (time == 0) {
        const char* str = NextTok(arg1, argEnd);
        SegmentDisplay_SetText(TokLen(str, argEnd), str, SegmentDisplaySlot_Macro);
        return MacroResult_Finished;
    } else if ((res = processDelay(time)) == MacroResult_Finished) {
        SegmentDisplay_DeactivateSlot(SegmentDisplaySlot_Macro);
        return MacroResult_Finished;
    } else {
        const char* str = NextTok(arg1, argEnd);
        SegmentDisplay_SetText(TokLen(str, argEnd), str, SegmentDisplaySlot_Macro);
        return res;
    }
}

static macro_result_t processSetRegCommand(const char* arg1, const char *argEnd)
{
    uint8_t address = parseNUM(arg1, argEnd);
    int32_t param = parseNUM(NextTok(arg1, argEnd), argEnd);
    if (validReg(address)) {
        regs[address] = param;
    }
    return MacroResult_Finished;
}

static macro_result_t processRegAddCommand(const char* arg1, const char *argEnd, bool invert)
{
    uint8_t address = parseNUM(arg1, argEnd);
    int32_t param = parseNUM(NextTok(arg1, argEnd), argEnd);
    if (validReg(address)) {
        if (invert) {
            regs[address] = regs[address] - param;
        } else {
            regs[address] = regs[address] + param;
        }
    }
    return MacroResult_Finished;
}

static macro_result_t processRegMulCommand(const char* arg1, const char *argEnd)
{
    uint8_t address = parseNUM(arg1, argEnd);
    int32_t param = parseNUM(NextTok(arg1, argEnd), argEnd);
    if (validReg(address)) {
        regs[address] = regs[address]*param;
    }
    return MacroResult_Finished;
}


static macro_result_t goToAddress(uint8_t address)
{
    if(address == s->ms.commandAddress) {
        memset(&s->as, 0, sizeof s->as);

        return MacroResult_JumpedBackward;
    }

    uint8_t oldAddress = s->ms.commandAddress;

    //if we jump back, we have to reset and go from beginning
    if (address < s->ms.commandAddress) {
        resetToAddressZero(s->ms.currentMacroIndex);
    }

    //if we are in the middle of multicommand action, parse till the end
    if(s->ms.commandAddress < address && s->ms.commandAddress != 0) {
        while (s->ms.commandAddress < address && loadNextCommand());
    }

    //skip across actions without having to read entire action
    uint8_t cmdCount = currentActionCmdCount();
    while (s->ms.commandAddress + cmdCount <= address) {
        loadNextAction();
        s->ms.commandAddress += cmdCount - 1; //loadNextAction already added one
        cmdCount = currentActionCmdCount();
    }

    //now go command by command
    while (s->ms.commandAddress < address && loadNextCommand()) ;

    return address > oldAddress ? MacroResult_JumpedForward: MacroResult_JumpedBackward;
}

static macro_result_t goToLabel(const char* arg, const char* argEnd)
{
    uint8_t startedAtAdr = s->ms.commandAddress;
    bool secondPass = false;
    bool reachedEnd = false;

    while ( !secondPass || s->ms.commandAddress < startedAtAdr ) {
        while ( !reachedEnd && (!secondPass || s->ms.commandAddress < startedAtAdr) ) {
            if(s->ms.currentMacroAction.type == MacroActionType_Command) {
                const char* cmd = s->ms.currentMacroAction.cmd.text + s->ms.commandBegin;
                const char* cmdEnd = s->ms.currentMacroAction.cmd.text + s->ms.commandEnd;
                const char* cmdTokEnd = TokEnd(cmd, cmdEnd);

                Macros_SetStatusNum(s->ms.commandAddress);
                Macros_SetStatusString("\n",  NULL);

                if(cmdTokEnd[-1] == ':' && TokenMatches2(cmd, cmdTokEnd-1, arg, argEnd)) {
                    return s->ms.commandAddress > startedAtAdr ? MacroResult_JumpedForward : MacroResult_JumpedBackward;
                }
            }

            reachedEnd = !loadNextCommand() && !loadNextAction();
        }

        if (!secondPass) {
            resetToAddressZero(s->ms.currentMacroIndex);
            secondPass = true;
            reachedEnd = false;
        }
    }

    Macros_ReportError("Label not found", arg, argEnd);
    s->ms.macroBroken = true;

    return MacroResult_Finished;
}

static macro_result_t goTo(const char* arg, const char* argEnd)
{
    if (isNUM(arg, argEnd)) {
        return goToAddress(parseNUM(arg, argEnd));
    } else {
        return goToLabel(arg, argEnd);
    }
}

static macro_result_t processGoToCommand(const char* arg, const char *argEnd)
{
    return goTo(arg, argEnd);
}

static macro_result_t processYieldCommand(const char* arg, const char *argEnd)
{
    return MacroResult_ActionFinishedFlag | MacroResult_YieldFlag;
}

static macro_result_t processStopRecordingCommand()
{
    MacroRecorder_StopRecording();
    return MacroResult_Finished;
}

static macro_result_t processMouseCommand(bool enable, const char* arg1, const char *argEnd)
{
    const char* arg2 = NextTok(arg1, argEnd);
    uint8_t dirOffset = 0;

    serialized_mouse_action_t baseAction = SerializedMouseAction_LeftClick;

    if (TokenMatches(arg1, argEnd, "move")) {
        baseAction = SerializedMouseAction_MoveUp;
    }
    else if (TokenMatches(arg1, argEnd, "scroll")) {
        baseAction = SerializedMouseAction_ScrollUp;
    }
    else if (TokenMatches(arg1, argEnd, "accelerate")) {
        baseAction = SerializedMouseAction_Accelerate;
    }
    else if (TokenMatches(arg1, argEnd, "decelerate")) {
        baseAction = SerializedMouseAction_Decelerate;
    }
    else {
        Macros_ReportError("unrecognized argument", arg1, argEnd);
    }

    if (baseAction == SerializedMouseAction_MoveUp || baseAction == SerializedMouseAction_ScrollUp) {
        if (TokenMatches(arg2, argEnd, "up")) {
            dirOffset = 0;
        }
        else if (TokenMatches(arg2, argEnd, "down")) {
            dirOffset = 1;
        }
        else if (TokenMatches(arg2, argEnd, "left")) {
            dirOffset = 2;
        }
        else if (TokenMatches(arg2, argEnd, "right")) {
            dirOffset = 3;
        }
        else {
            Macros_ReportError("unrecognized argument", arg2, argEnd);
        }
    }

    if (baseAction != SerializedMouseAction_LeftClick) {
        ToggleMouseState(baseAction + dirOffset, enable);
    }
    return MacroResult_Finished;
}

static macro_result_t processRecordMacroCommand(const char* arg, const char *argEnd, bool blind)
{
    uint16_t id = parseRuntimeMacroSlotId(arg, argEnd);
    MacroRecorder_RecordRuntimeMacroSmart(id, blind);
    return MacroResult_Finished;
}

static macro_result_t processStartRecordingCommand(const char* arg, const char *argEnd, bool blind)
{
    uint16_t id = parseRuntimeMacroSlotId(arg, argEnd);
    MacroRecorder_StartRecording(id, blind);
    return MacroResult_Finished;
}

static macro_result_t processPlayMacroCommand(const char* arg, const char *argEnd)
{
    s->ms.reportsUsed = true;
    uint16_t id = parseRuntimeMacroSlotId(arg, argEnd);
    bool res = MacroRecorder_PlayRuntimeMacroSmart(id, &s->ms.macroBasicKeyboardReport);
    return res ? MacroResult_Blocking : MacroResult_Finished;
}

static macro_result_t processWriteCommand(const char* arg, const char *argEnd)
{
    // todo: clean this up when refactoring write tokenization
    while (argEnd > arg && (argEnd[-1] == '\n' || argEnd[-1] == '\r')) {
        argEnd--;
    }

    return dispatchText(arg, argEnd - arg);
}


static macro_result_t processWriteExprCommand(const char* arg, const char *argEnd)
{
    uint32_t num = parseNUM(arg, argEnd);
    return writeNum(num);
}

static void processSuppressModsCommand()
{
    SuppressMods = true;
    s->as.modifierSuppressMods = true;
}

static void processPostponeKeysCommand()
{
    postponeCurrentCycle();
    s->as.modifierSuppressMods = true;
}

static macro_result_t processStatsRuntimeCommand()
{
    int ms = Timer_GetElapsedTime(&s->ms.currentMacroStartTime);
    Macros_SetStatusString("macro runtime is: ", NULL);
    Macros_SetStatusNum(ms);
    Macros_SetStatusString(" ms\n", NULL);
    return MacroResult_Finished;
}


static macro_result_t processNoOpCommand()
{
    if (!s->as.actionActive) {
        s->as.actionActive = true;
        return MacroResult_Blocking;
    } else {
        s->as.actionActive = false;
        return MacroResult_Finished;
    }
}

#define RESOLVESEC_RESULT_DONTKNOWYET 0
#define RESOLVESEC_RESULT_PRIMARY 1
#define RESOLVESEC_RESULT_SECONDARY 2


static uint8_t processResolveSecondary(uint16_t timeout1, uint16_t timeout2)
{
    postponeCurrentCycle();
    bool pendingReleased = PostponerExtended_IsPendingKeyReleased(0);
    bool currentKeyIsActive = currentMacroKeyIsActive();

    //phase 1 - wait until some other key is released, then write down its release time
    bool timer1Exceeded = Timer_GetElapsedTime(&s->ms.currentMacroStartTime) >= timeout1;
    if (!timer1Exceeded && currentKeyIsActive && !pendingReleased) {
        s->as.secondaryRoleData.phase2Start = 0;
        return RESOLVESEC_RESULT_DONTKNOWYET;
    }
    if (s->as.secondaryRoleData.phase2Start == 0) {
        s->as.secondaryRoleData.phase2Start = CurrentTime;
    }
    //phase 2 - "safety margin" - wait another `timeout2` ms, and if the switcher is released during this time, still interpret it as a primary action
    bool timer2Exceeded = Timer_GetElapsedTime(&s->as.secondaryRoleData.phase2Start) >= timeout2;
    if (!timer1Exceeded && !timer2Exceeded &&  currentKeyIsActive && pendingReleased && PostponerQuery_PendingKeypressCount() < 3) {
        return RESOLVESEC_RESULT_DONTKNOWYET;
    }
    //phase 3 - resolve the situation - if the switcher is released first or within the "safety margin", interpret it as primary action, otherwise secondary
    if (timer1Exceeded || (pendingReleased && timer2Exceeded)) {
        return RESOLVESEC_RESULT_SECONDARY;
    }
    else {
        return RESOLVESEC_RESULT_PRIMARY;
    }

}

static macro_result_t processResolveSecondaryCommand(const char* arg1, const char* argEnd)
{

    const char* arg2 = NextTok(arg1, argEnd);
    const char* arg3 = NextTok(arg2, argEnd);
    const char* arg4 = NextTok(arg3, argEnd);

    const char* primaryAdr;
    const char* secondaryAdr;
    uint16_t timeout1;
    uint16_t timeout2;

    if (arg4 == argEnd) {
        timeout1 = parseNUM(arg1, argEnd);
        timeout2 = timeout1;
        primaryAdr = arg2;
        secondaryAdr = arg3;
    } else {
        timeout1 = parseNUM(arg1, argEnd);
        timeout2 = parseNUM(arg2, argEnd);
        primaryAdr = arg3;
        secondaryAdr = arg4;
    }

    uint8_t res = processResolveSecondary(timeout1, timeout2);

    switch(res) {
    case RESOLVESEC_RESULT_DONTKNOWYET:
        return MacroResult_Waiting;
    case RESOLVESEC_RESULT_PRIMARY:
        postponeNextN(1);
        Macros_ReportWarn("Command deprecated. Please, replace resolveSecondary by `ifPrimary advancedStrategy goTo ...` or `ifSecondary advancedStrategy goTo ...`.", NULL, NULL);
        return goTo(primaryAdr, argEnd);
    case RESOLVESEC_RESULT_SECONDARY:
        return goTo(secondaryAdr, argEnd);
    }
    //this is unreachable, prevents warning
    return MacroResult_Finished;
}



static macro_result_t processIfSecondaryCommand(bool negate, const char* arg, const char* argEnd)
{
    secondary_role_strategy_t strategy = SecondaryRoles_Strategy;

    if (TokenMatches(arg, argEnd, "simpleStrategy")) {
        arg = NextTok(arg, argEnd);
        strategy = SecondaryRoleStrategy_Simple;
    }
    if (TokenMatches(arg, argEnd, "advancedStrategy")) {
        arg = NextTok(arg, argEnd);
        strategy = SecondaryRoleStrategy_Advanced;
    }

    if (s->as.currentIfSecondaryConditionPassed) {
        if (s->as.currentConditionPassed) {
            goto conditionPassed;
        } else {
            s->as.currentIfSecondaryConditionPassed = false;
        }
    }

    postponeCurrentCycle();
    secondary_role_result_t res = SecondaryRoles_ResolveState(s->ms.currentMacroKey, 0, strategy, !s->as.actionActive);

    s->as.actionActive = res.state == SecondaryRoleState_DontKnowYet;

    switch(res.state) {
    case SecondaryRoleState_DontKnowYet:
        return MacroResult_Waiting;
    case SecondaryRoleState_Primary:
        if (negate) {
            goto conditionPassed;
        } else {
            postponeNextN(0);
            return MacroResult_Finished;
        }
    case SecondaryRoleState_Secondary:
        if (negate) {
            return MacroResult_Finished;
        } else {
            goto conditionPassed;
        }
    }
conditionPassed:
    s->as.currentIfSecondaryConditionPassed = true;
    s->as.currentConditionPassed = false; //otherwise following conditions would be skipped
    return processCommand(arg, argEnd);
}

static macro_action_t decodeKey(const char* arg1, const char* argEnd, macro_sub_action_t defaultSubAction)
{
    macro_action_t action;
    MacroShortcutParser_Parse(arg1, TokEnd(arg1, argEnd), defaultSubAction, &action, NULL);

    return action;
}

static macro_result_t processKeyCommand(macro_sub_action_t type, const char* arg1, const char* argEnd)
{
    macro_action_t action = decodeKey(arg1, argEnd, type);

    switch (action.type) {
        case MacroActionType_Key:
            return processKey(action);
        case MacroActionType_MouseButton:
            return processMouseButton(action);
        default:
            return MacroResult_Finished;
    }
}

static macro_result_t processTapKeySeqCommand(const char* arg1, const char* argEnd)
{
    for(uint8_t i = 0; i < s->as.keySeqData.atKeyIdx; i++) {
        arg1 = NextTok(arg1, argEnd);

        if(arg1 == argEnd) {
            s->as.keySeqData.atKeyIdx = 0;
            return MacroResult_Finished;
        };
    }

    macro_result_t res = processKeyCommand(MacroSubAction_Tap, arg1, argEnd);

    if(res == MacroResult_Finished) {
        s->as.keySeqData.atKeyIdx++;
    }

    return res == MacroResult_Waiting ? MacroResult_Waiting : MacroResult_Blocking;
}

static macro_result_t processResolveNextKeyIdCommand()
{
    postponeCurrentCycle();
    if (PostponerQuery_PendingKeypressCount() == 0) {
        return MacroResult_Waiting;
    }
    macro_result_t res = writeNum(PostponerExtended_PendingId(0));
    if (res == MacroResult_Finished) {
        PostponerExtended_ConsumePendingKeypresses(1, true);
        return MacroResult_Finished;
    }
    return res;
}

static macro_result_t processResolveNextKeyEqCommand(const char* arg1, const char* argEnd)
{
    Macros_ReportWarn("Command deprecated. Please, replace resolveNextKeyEq by ifShortcut or ifGesture, or complain at github that you actually need this.", NULL, NULL);
    postponeCurrentCycle();
    const char* arg2 = NextTok(arg1, argEnd);
    const char* arg3 = NextTok(arg2, argEnd);
    const char* arg4 = NextTok(arg3, argEnd);
    const char* arg5 = NextTok(arg4, argEnd);
    uint16_t idx = parseNUM(arg1, argEnd);
    uint16_t key = parseNUM(arg2, argEnd);
    uint16_t timeout = 0;
    bool untilRelease = false;
    if (TokenMatches(arg3, argEnd, "untilRelease")) {
       untilRelease = true;
    } else {
       timeout = parseNUM(arg3, argEnd);
    }
    const char* adr1 = arg4;
    const char* adr2 = arg5;


    if (idx > POSTPONER_BUFFER_MAX_FILL) {
        Macros_ReportErrorNum("Invalid argument 1, allowed at most: ", idx);
    }

    if (untilRelease ? !currentMacroKeyIsActive() : Timer_GetElapsedTime(&s->ms.currentMacroStartTime) >= timeout) {
        return goTo(adr2, argEnd);
    }
    if (PostponerQuery_PendingKeypressCount() < idx + 1) {
        return MacroResult_Waiting;
    }

    if (PostponerExtended_PendingId(idx) == key) {
        return goTo(adr1, argEnd);
    } else {
        return goTo(adr2, argEnd);
    }
}

static macro_result_t processIfShortcutCommand(bool negate, const char* arg, const char* argEnd, bool untilRelease)
{
    //parse optional flags
    bool consume = true;
    bool transitive = false;
    bool fixedOrder = true;
    bool orGate = false;
    uint16_t cancelIn = 0;
    uint16_t timeoutIn= 0;
    while(arg < argEnd && !isNUM(arg, argEnd)) {
        if (TokenMatches(arg, argEnd, "noConsume")) {
            arg = NextTok(arg, argEnd);
            consume = false;
        } else if (TokenMatches(arg, argEnd, "transitive")) {
            arg = NextTok(arg, argEnd);
            transitive = true;
        } else if (TokenMatches(arg, argEnd, "timeoutIn")) {
            arg = NextTok(arg, argEnd);
            timeoutIn = parseNUM(arg, argEnd);
            arg = NextTok(arg, argEnd);
        } else if (TokenMatches(arg, argEnd, "cancelIn")) {
            arg = NextTok(arg, argEnd);
            cancelIn = parseNUM(arg, argEnd);
            arg = NextTok(arg, argEnd);
        } else if (TokenMatches(arg, argEnd, "anyOrder")) {
            arg = NextTok(arg, argEnd);
            fixedOrder = false;
        } else if (TokenMatches(arg, argEnd, "orGate")) {
            arg = NextTok(arg, argEnd);
            orGate = true;
        } else {
            Macros_ReportError("Unrecognized option", arg, argEnd);
            arg = NextTok(arg, argEnd);
        }
    }

    //
    if (s->as.currentIfShortcutConditionPassed) {
        if (s->as.currentConditionPassed) {
            goto conditionPassed;
        } else {
            s->as.currentIfShortcutConditionPassed = false;
        }
    }

    //parse and check KEYIDs
    postponeCurrentCycle();
    uint8_t pendingCount = PostponerQuery_PendingKeypressCount();
    uint8_t numArgs = 0;
    bool someoneNotReleased = false;
    while(isNUM(arg, argEnd) && arg < argEnd) {
        numArgs++;
        uint8_t argKeyid = parseNUM(arg, argEnd);
        arg = NextTok(arg, argEnd);
        if (pendingCount < numArgs) {
            uint32_t referenceTime = transitive && pendingCount > 0 ? PostponerExtended_LastPressTime() : s->ms.currentMacroStartTime;
            uint16_t elapsedSinceReference = Timer_GetElapsedTime(&referenceTime);

            bool shortcutTimedOut = untilRelease && !currentMacroKeyIsActive() && (!transitive || !someoneNotReleased);
            bool gestureDefaultTimedOut = !untilRelease && cancelIn == 0 && timeoutIn == 0 && elapsedSinceReference > 1000;
            bool cancelInTimedOut = cancelIn != 0 && elapsedSinceReference > cancelIn;
            bool timeoutInTimedOut = timeoutIn != 0 && elapsedSinceReference > timeoutIn;
            if (!shortcutTimedOut && !gestureDefaultTimedOut && !cancelInTimedOut && !timeoutInTimedOut) {
                if (!untilRelease && cancelIn == 0 && timeoutIn == 0) {
                    sleepTillTime(referenceTime+1000);
                }
                if (cancelIn != 0) {
                    sleepTillTime(referenceTime+cancelIn);
                }
                if (timeoutIn != 0) {
                    sleepTillTime(referenceTime+timeoutIn);
                }
                sleepTillKeystateChange();
                return MacroResult_Sleeping;
            }
            else if (cancelInTimedOut) {
                PostponerExtended_ConsumePendingKeypresses(numArgs, true);
                s->ms.macroBroken = true;
                return MacroResult_Finished;
            }
            else {
                if (negate) {
                    goto conditionPassed;
                } else {
                    return MacroResult_Finished;
                }
            }
        }
        else if (orGate) {
            // go through all canidates all at once
            while (true) {
                // first keyid had already been processed.
                if (PostponerQuery_ContainsKeyId(argKeyid)) {
                    if (negate) {
                        return MacroResult_Finished;
                    } else {
                        goto conditionPassed;
                    }
                }
                if (!(isNUM(arg, argEnd) && arg < argEnd)) {
                    break;
                }
                argKeyid = parseNUM(arg, argEnd);
                arg = NextTok(arg, argEnd);
            }
            // none is matched
            if (negate) {
                goto conditionPassed;
            } else {
                return MacroResult_Finished;
            }
        }
        else if (fixedOrder && PostponerExtended_PendingId(numArgs - 1) != argKeyid) {
            if (negate) {
                goto conditionPassed;
            } else {
                return MacroResult_Finished;
            }
        }
        else if (!fixedOrder && !PostponerQuery_ContainsKeyId(argKeyid)) {
            if (negate) {
                goto conditionPassed;
            } else {
                return MacroResult_Finished;
            }
        }
        else {
            someoneNotReleased |= !PostponerQuery_IsKeyReleased(Utils_KeyIdToKeyState(argKeyid));
        }
    }
    //all keys match
    if (consume) {
        PostponerExtended_ConsumePendingKeypresses(numArgs, true);
    }
    if (negate) {
        return MacroResult_Finished;
    } else {
        goto conditionPassed;
    }
conditionPassed:
    while(isNUM(arg, argEnd) && arg < argEnd) {
        arg = NextTok(arg, argEnd);
    }
    s->as.currentIfShortcutConditionPassed = true;
    s->as.currentConditionPassed = false; //otherwise following conditions would be skipped
    return processCommand(arg, argEnd);
}

static macro_result_t processAutoRepeatCommand(const char* arg1, const char* argEnd) {
    switch (s->ms.autoRepeatPhase) {
    case AutoRepeatState_Waiting:
        goto process_delay;
    case AutoRepeatState_Executing:
    default:
        goto run_command;
    }

process_delay:;
    uint16_t delay = s->ms.autoRepeatInitialDelayPassed ? AutoRepeatDelayRate : AutoRepeatInitialDelay;
    bool pendingReleased = PostponerQuery_IsKeyReleased(s->ms.currentMacroKey);
    bool currentKeyIsActive = currentMacroKeyIsActive();

    if (!currentKeyIsActive || pendingReleased) {
        // reset delay state in case it was interrupted by key release
        memset(&s->as.delayData, 0, sizeof s->as.delayData);
        s->as.actionActive = 0;
        s->ms.autoRepeatPhase = AutoRepeatState_Executing;

        return MacroResult_Finished;
    }

    if (processDelay(delay) == MacroResult_Finished) {
        s->ms.autoRepeatInitialDelayPassed = true;
        s->ms.autoRepeatPhase = AutoRepeatState_Executing;
        goto run_command;
    } else {
        sleepTillKeystateChange();
        return MacroResult_Sleeping;
    }


run_command:;
    macro_result_t res = processCommand(arg1, argEnd);
    if (res & MacroResult_ActionFinishedFlag) {
        s->ms.autoRepeatPhase = AutoRepeatState_Waiting;
        //tidy the state in case someone left it dirty
        memset(&s->as.delayData, 0, sizeof s->as.delayData);
        s->as.actionActive = false;
        s->as.actionPhase = 0;
        return (res & ~MacroResult_ActionFinishedFlag) | MacroResult_InProgressFlag;
    } else if (res & MacroResult_DoneFlag) {
        s->ms.autoRepeatPhase = AutoRepeatState_Waiting;
        return res;
    } else {
        return res;
    }
}


static macro_result_t processOneShotCommand(const char* arg1, const char* argEnd) {
    /* In order for modifiers of a scancode action to be applied properly, we need to
     * make the action live at least one cycle after macroInterrupted becomes true.
     *
     * Also, we need to prevent the command to go sleeping after this because
     * we would not be woken up.
     * */
    if (!s->ms.macroInterrupted) {
        s->ms.oneShotState = 1;
    } else if (s->ms.oneShotState < 3) {
        s->ms.oneShotState++;
    } else {
        s->ms.oneShotState = 0;
    }
    macro_result_t res = processCommand(arg1, argEnd);
    if (res & MacroResult_ActionFinishedFlag || res & MacroResult_DoneFlag) {
        s->ms.oneShotState = 0;
    }
    if (s->ms.oneShotState > 1) {
        return res | MacroResult_Blocking;
    } else {
        return res;
    }
}

static macro_result_t processOverlayKeymapCommand(const char* arg1, const char* argEnd)
{
    uint8_t srcKeymapId = parseKeymapId(arg1, argEnd);

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }

    OverlayKeymap(srcKeymapId);
    return MacroResult_Finished;
}

static macro_result_t processReplaceLayerCommand(const char* arg1, const char* argEnd)
{
    const char* arg2 = NextTok(arg1, argEnd);
    const char* arg3 = NextTok(arg2, argEnd);
    layer_id_t dstLayerId = Macros_ParseLayerId(arg1, argEnd);
    uint8_t srcKeymapId = parseKeymapId(arg2, argEnd);
    layer_id_t srcLayerId = Macros_ParseLayerId(arg3, argEnd);

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }

    ReplaceLayer(dstLayerId, srcKeymapId, srcLayerId);
    return MacroResult_Finished;
}

static macro_result_t processOverlayLayerCommand(const char* arg1, const char* argEnd)
{
    const char* arg2 = NextTok(arg1, argEnd);
    const char* arg3 = NextTok(arg2, argEnd);
    layer_id_t dstLayerId = Macros_ParseLayerId(arg1, argEnd);
    uint8_t srcKeymapId = parseKeymapId(arg2, argEnd);
    layer_id_t srcLayerId = Macros_ParseLayerId(arg3, argEnd);

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }

    OverlayLayer(dstLayerId, srcKeymapId, srcLayerId);
    return MacroResult_Finished;
}

static bool processIfKeyPendingAtCommand(bool negate, const char* arg1, const char* argEnd)
{
    const char* arg2 = NextTok(arg1, argEnd);
    uint16_t idx = parseNUM(arg1, argEnd);
    uint16_t key = parseNUM(arg2, argEnd);

    return (PostponerExtended_PendingId(idx) == key) != negate;
}

static bool processIfKeyActiveCommand(bool negate, const char* arg1, const char* argEnd)
{
    uint16_t keyid = parseNUM(arg1, argEnd);
    key_state_t* key = Utils_KeyIdToKeyState(keyid);
    return KeyState_Active(key) != negate;
}

static bool processIfPendingKeyReleasedCommand(bool negate, const char* arg1, const char* argEnd)
{
    uint16_t idx = parseNUM(arg1, argEnd);
    return PostponerExtended_IsPendingKeyReleased(idx) != negate;
}

static bool processIfKeyDefinedCommand(bool negate, const char* arg1, const char* argEnd)
{
    uint16_t keyid = parseNUM(arg1, argEnd);
    uint8_t slot;
    uint8_t slotIdx;
    Utils_DecodeId(keyid, &slot, &slotIdx);
    key_action_t* action = &CurrentKeymap[ActiveLayer][slot][slotIdx];
    return (action->type != KeyActionType_None) != negate;
}

static macro_result_t processActivateKeyPostponedCommand(const char* arg1, const char* argEnd)
{
    uint8_t layer = 255;
    bool options = true;
    bool append = true;

    while (options) {
        options = false;
        if (TokenMatches(arg1, argEnd, "atLayer")) {
            const char* arg2 = NextTok(arg1, argEnd);
            layer = Macros_ParseLayerId(arg2, argEnd);
            arg1 = NextTok(arg2, argEnd);
            options = true;
        }
        if (TokenMatches(arg1, argEnd, "append")) {
            arg1 = NextTok(arg1, argEnd);
            append = true;
            options = true;
        }
        if (TokenMatches(arg1, argEnd, "prepend")) {
            arg1 = NextTok(arg1, argEnd);
            append = false;
            options = true;
        }
    }

    if (Macros_ParserError) {
        return MacroResult_Finished;
    }

    uint16_t keyid = parseNUM(arg1, argEnd);
    key_state_t* key = Utils_KeyIdToKeyState(keyid);

    if (append) {
        if (PostponerQuery_IsActiveEventually(key)) {
            PostponerCore_TrackKeyEvent(key, false, layer);
            PostponerCore_TrackKeyEvent(key, true, layer);
        } else {
            PostponerCore_TrackKeyEvent(key, true, layer);
            PostponerCore_TrackKeyEvent(key, false, layer);
        }
    } else {
        if (KeyState_Active(key)) {
            //reverse order when prepending
            PostponerCore_PrependKeyEvent(key, true, layer);
            PostponerCore_PrependKeyEvent(key, false, layer);
        } else {
            PostponerCore_PrependKeyEvent(key, false, layer);
            PostponerCore_PrependKeyEvent(key, true, layer);
        }
    }
    return MacroResult_Finished;
}

static macro_result_t processConsumePendingCommand(const char* arg1, const char* argEnd)
{
    uint16_t cnt = parseNUM(arg1, argEnd);
    PostponerExtended_ConsumePendingKeypresses(cnt, true);
    return MacroResult_Finished;
}

static macro_result_t processPostponeNextNCommand(const char* arg1, const char* argEnd)
{
    uint16_t cnt = parseNUM(arg1, argEnd);
    PostponerCore_PostponeNCycles(MACRO_CYCLES_TO_POSTPONE);
    postponeNextN(cnt);
    return MacroResult_Finished;
}


static macro_result_t processRepeatForCommand(const char* arg1, const char* argEnd)
{
    uint8_t idx = parseNUM(arg1, argEnd);
    const char* adr = NextTok(arg1, argEnd);
    if (validReg(idx)) {
        if (regs[idx] > 0) {
            regs[idx]--;
            if (regs[idx] > 0) {
                return goTo(adr, argEnd);
            }
        }
    }
    return MacroResult_Finished;
}

static macro_result_t processResetTrackpointCommand()
{
    UhkModuleSlaveDriver_ResetTrackpoint();
    return MacroResult_Finished;
}

static macro_result_t processExecCommand(const char* arg1, const char* cmdEnd)
{
    uint8_t macroIndex = FindMacroIndexByName(arg1, TokEnd(arg1, cmdEnd), true);
    return execMacro(macroIndex);
}

static macro_result_t processCallCommand(const char* arg1, const char* cmdEnd)
{
    uint8_t macroIndex = FindMacroIndexByName(arg1, TokEnd(arg1, cmdEnd), true);
    return callMacro(macroIndex);
}

static macro_result_t processForkCommand(const char* arg1, const char* cmdEnd)
{
    uint8_t macroIndex = FindMacroIndexByName(arg1, TokEnd(arg1, cmdEnd), true);
    return forkMacro(macroIndex);
}

static macro_result_t processProgressHueCommand()
{
#define C(I) (*cols[((phase + I + 3) % 3)])

    const uint8_t step = 10;
    static uint8_t phase = 0;

    uint8_t* cols[] = {
        &LedMap_ConstantRGB.red,
        &LedMap_ConstantRGB.green,
        &LedMap_ConstantRGB.blue
    };

    bool progressed = false;
    if (C(-1) > step) {
        C(-1) -= step;
        progressed = true;
    }
    if (C(0) < 255-step+1) {
        C(0) += step;
        progressed = true;
    }
    if (!progressed) {
        C(-1) = 0;
        C(0) = 255;
        phase++;
    }

    Ledmap_SetLedBacklightingMode(BacklightingMode_ConstantRGB);
    Ledmap_UpdateBacklightLeds();
    return MacroResult_Finished;
#undef C
}

static macro_result_t processCommand(const char* cmd, const char* cmdEnd)
{
    if (*cmd == '$') {
        cmd++;
    }

    const char* cmdTokEnd = TokEnd(cmd, cmdEnd);
    if (cmdTokEnd > cmd && cmdTokEnd[-1] == ':') {
        //skip labels
        cmd = NextTok(cmd, cmdEnd);
        if (cmd == cmdEnd) {
            return MacroResult_Finished;
        }
    }
    while(*cmd && cmd < cmdEnd) {
        const char* arg1 = NextTok(cmd, cmdEnd);
        switch(*cmd) {
        case 'a':
            if (TokenMatches(cmd, cmdEnd, "addReg")) {
                return processRegAddCommand(arg1, cmdEnd, false);
            }
            else if (TokenMatches(cmd, cmdEnd, "activateKeyPostponed")) {
                return processActivateKeyPostponedCommand(arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "autoRepeat")) {
                return processAutoRepeatCommand(NextTok(cmd, cmdEnd), cmdEnd);
            }
            else {
                goto failed;
            }
            break;
        case 'b':
            if (TokenMatches(cmd, cmdEnd, "break")) {
                return processBreakCommand();
            }
            else {
                goto failed;
            }
            break;
        case 'c':
            if (TokenMatches(cmd, cmdEnd, "consumePending")) {
                return processConsumePendingCommand(arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "clearStatus")) {
                return processClearStatusCommand();
            }
            else if (TokenMatches(cmd, cmdEnd, "call")) {
                return processCallCommand(arg1, cmdEnd);
            }
            else {
                goto failed;
            }
            break;
        case 'd':
            if (TokenMatches(cmd, cmdEnd, "delayUntilRelease")) {
                return processDelayUntilReleaseCommand();
            }
            else if (TokenMatches(cmd, cmdEnd, "delayUntilReleaseMax")) {
                return processDelayUntilReleaseMaxCommand(arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "delayUntil")) {
                return processDelayUntilCommand(arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "diagnose")) {
                return processDiagnoseCommand();
            }
            else {
                goto failed;
            }
            break;
        case 'e':
            if (TokenMatches(cmd, cmdEnd, "exec")) {
                return processExecCommand(arg1, cmdEnd);
            }
            else {
                goto failed;
            }
            break;
        case 'f':
            if (TokenMatches(cmd, cmdEnd, "final")) {
                macro_result_t res = processCommand(NextTok(cmd, cmdEnd), cmdEnd);
                if (res & MacroResult_InProgressFlag) {
                    return res;
                } else {
                    s->ms.macroBroken = true;
                    return MacroResult_Finished;
                }
            }
            else if (TokenMatches(cmd, cmdEnd, "fork")) {
                return processForkCommand(arg1, cmdEnd);
            }
            else {
                goto failed;
            }
            break;
        case 'g':
            if (TokenMatches(cmd, cmdEnd, "goTo")) {
                return processGoToCommand(arg1, cmdEnd);
            }
            else {
                goto failed;
            }
            break;
        case 'h':
            if (TokenMatches(cmd, cmdEnd, "holdLayer")) {
                return processHoldLayerCommand(arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "holdLayerMax")) {
                return processHoldLayerMaxCommand(arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "holdKeymapLayer")) {
                return processHoldKeymapLayerCommand(arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "holdKeymapLayerMax")) {
                return processHoldKeymapLayerMaxCommand(arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "holdKey")) {
                return processKeyCommand(MacroSubAction_Hold, arg1, cmdEnd);
            }
            else {
                goto failed;
            }
            break;
        case 'i':
            if (TokenMatches(cmd, cmdEnd, "ifDoubletap")) {
                if (!processIfDoubletapCommand(false) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
            }
            else if (TokenMatches(cmd, cmdEnd, "ifNotDoubletap")) {
                if (!processIfDoubletapCommand(true) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
            }
            else if (TokenMatches(cmd, cmdEnd, "ifInterrupted")) {
                if (!processIfInterruptedCommand(false) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
            }
            else if (TokenMatches(cmd, cmdEnd, "ifNotInterrupted")) {
                if (!processIfInterruptedCommand(true) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
            }
            else if (TokenMatches(cmd, cmdEnd, "ifReleased")) {
                if (!processIfReleasedCommand(false) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
            }
            else if (TokenMatches(cmd, cmdEnd, "ifNotReleased")) {
                if (!processIfReleasedCommand(true) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
            }
            else if (TokenMatches(cmd, cmdEnd, "ifRegEq")) {
                if (!processIfRegEqCommand(false, arg1, cmdEnd) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
                cmd = NextTok(arg1, cmdEnd); //shift by 2
                arg1 = NextTok(cmd, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "ifNotRegEq")) {
                if (!processIfRegEqCommand(true, arg1, cmdEnd) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
                cmd = NextTok(arg1, cmdEnd); //shift by 2
                arg1 = NextTok(cmd, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "ifRegGt")) {
                if (!processIfRegInequalityCommand(true, arg1, cmdEnd) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
                cmd = NextTok(arg1, cmdEnd); //shift by 2
                arg1 = NextTok(cmd, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "ifRegLt")) {
                if (!processIfRegInequalityCommand(false, arg1, cmdEnd) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
                cmd = NextTok(arg1, cmdEnd); //shift by 2
                arg1 = NextTok(cmd, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "ifKeymap")) {
                if (!processIfKeymapCommand(false, arg1, cmdEnd) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
                cmd = arg1; //shift by 1
                arg1 = NextTok(cmd, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "ifNotKeymap")) {
                if (!processIfKeymapCommand(true, arg1, cmdEnd) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
                cmd = arg1; //shift by 1
                arg1 = NextTok(cmd, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "ifLayer")) {
                if (!processIfLayerCommand(false, arg1, cmdEnd) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
                cmd = arg1; //shift by 1
                arg1 = NextTok(cmd, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "ifNotLayer")) {
                if (!processIfLayerCommand(true, arg1, cmdEnd) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
                cmd = arg1; //shift by 1
                arg1 = NextTok(cmd, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "ifPlaytime")) {
                if (!processIfPlaytimeCommand(false, arg1, cmdEnd) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
                cmd = arg1;  //shift by 1
                arg1 = NextTok(cmd, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "ifNotPlaytime")) {
                if (!processIfPlaytimeCommand(true, arg1, cmdEnd) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
                cmd = arg1;
                arg1 = NextTok(cmd, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "ifAnyMod")) {
                if (!processIfModifierCommand(false, 0xFF)  && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
            }
            else if (TokenMatches(cmd, cmdEnd, "ifNotAnyMod")) {
                if (!processIfModifierCommand(true, 0xFF)  && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
            }
            else if (TokenMatches(cmd, cmdEnd, "ifShift")) {
                if (!processIfModifierCommand(false, SHIFTMASK)  && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
            }
            else if (TokenMatches(cmd, cmdEnd, "ifNotShift")) {
                if (!processIfModifierCommand(true, SHIFTMASK) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
            }
            else if (TokenMatches(cmd, cmdEnd, "ifCtrl")) {
                if (!processIfModifierCommand(false, CTRLMASK) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
            }
            else if (TokenMatches(cmd, cmdEnd, "ifNotCtrl")) {
                if (!processIfModifierCommand(true, CTRLMASK) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
            }
            else if (TokenMatches(cmd, cmdEnd, "ifAlt")) {
                if (!processIfModifierCommand(false, ALTMASK) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
            }
            else if (TokenMatches(cmd, cmdEnd, "ifNotAlt")) {
                if (!processIfModifierCommand(true, ALTMASK) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
            }
            else if (TokenMatches(cmd, cmdEnd, "ifGui")) {
                if (!processIfModifierCommand(false, GUIMASK)  && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
            }
            else if (TokenMatches(cmd, cmdEnd, "ifNotGui")) {
                if (!processIfModifierCommand(true, GUIMASK) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
            }
            else if (TokenMatches(cmd, cmdEnd, "ifCapsLockOn")) {
                if (!processIfStateKeyCommand(false, &UsbBasicKeyboard_CapsLockOn) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
            }
            else if (TokenMatches(cmd, cmdEnd, "ifNotCapsLockOn")) {
                if (!processIfStateKeyCommand(true, &UsbBasicKeyboard_CapsLockOn) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
            }
            else if (TokenMatches(cmd, cmdEnd, "ifNumLockOn")) {
                if (!processIfStateKeyCommand(false, &UsbBasicKeyboard_NumLockOn) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
            }
            else if (TokenMatches(cmd, cmdEnd, "ifNotNumLockOn")) {
                if (!processIfStateKeyCommand(true, &UsbBasicKeyboard_NumLockOn) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
            }
            else if (TokenMatches(cmd, cmdEnd, "ifScrollLockOn")) {
                if (!processIfStateKeyCommand(false, &UsbBasicKeyboard_ScrollLockOn) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
            }
            else if (TokenMatches(cmd, cmdEnd, "ifNotScrollLockOn")) {
                if (!processIfStateKeyCommand(true, &UsbBasicKeyboard_ScrollLockOn) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
            }
            else if (TokenMatches(cmd, cmdEnd, "ifRecording")) {
                if (!processIfRecordingCommand(false) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
            }
            else if (TokenMatches(cmd, cmdEnd, "ifNotRecording")) {
                if (!processIfRecordingCommand(true) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
            }
            else if (TokenMatches(cmd, cmdEnd, "ifRecordingId")) {
                if (!processIfRecordingIdCommand(false, arg1, cmdEnd) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
                cmd = arg1;
                arg1 = NextTok(cmd, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "ifNotRecordingId")) {
                if (!processIfRecordingIdCommand(true, arg1, cmdEnd) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
                cmd = arg1;
                arg1 = NextTok(cmd, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "ifNotPending")) {
                if (!processIfPendingCommand(true, arg1, cmdEnd) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
                cmd = arg1;
                arg1 = NextTok(cmd, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "ifPending")) {
                if (!processIfPendingCommand(false, arg1, cmdEnd) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
                cmd = arg1;
                arg1 = NextTok(cmd, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "ifKeyPendingAt")) {
                if (!processIfKeyPendingAtCommand(false, arg1, cmdEnd) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
                //shift by two
                cmd = NextTok(arg1, cmdEnd);
                arg1 = NextTok(cmd, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "ifNotKeyPendingAt")) {
                if (!processIfKeyPendingAtCommand(true, arg1, cmdEnd) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
                //shift by two
                cmd = NextTok(arg1, cmdEnd);
                arg1 = NextTok(cmd, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "ifKeyActive")) {
                if (!processIfKeyActiveCommand(false, arg1, cmdEnd) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
                cmd = arg1;
                arg1 = NextTok(cmd, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "ifNotKeyActive")) {
                if (!processIfKeyActiveCommand(true, arg1, cmdEnd) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
                cmd = arg1;
                arg1 = NextTok(cmd, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "ifPendingKeyReleased")) {
                if (!processIfPendingKeyReleasedCommand(false, arg1, cmdEnd) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
                cmd = arg1;
                arg1 = NextTok(cmd, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "ifNotPendingKeyReleased")) {
                if (!processIfPendingKeyReleasedCommand(true, arg1, cmdEnd) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
                cmd = arg1;
                arg1 = NextTok(cmd, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "ifKeyDefined")) {
                if (!processIfKeyDefinedCommand(false, arg1, cmdEnd) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
                cmd = arg1;
                arg1 = NextTok(cmd, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "ifNotKeyDefined")) {
                if (!processIfKeyDefinedCommand(true, arg1, cmdEnd) && !s->as.currentConditionPassed) {
                    return MacroResult_Finished;
                }
                cmd = arg1;
                arg1 = NextTok(cmd, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "ifSecondary")) {
                return processIfSecondaryCommand(false, arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "ifPrimary")) {
                return processIfSecondaryCommand(true, arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "ifShortcut")) {
                return processIfShortcutCommand(false, arg1, cmdEnd, true);
            }
            else if (TokenMatches(cmd, cmdEnd, "ifNotShortcut")) {
                return processIfShortcutCommand(true, arg1, cmdEnd, true);
            }
            else if (TokenMatches(cmd, cmdEnd, "ifGesture")) {
                return processIfShortcutCommand(false, arg1, cmdEnd, false);
            }
            else if (TokenMatches(cmd, cmdEnd, "ifNotGesture")) {
                return processIfShortcutCommand(true, arg1, cmdEnd, false);
            }
            else {
                goto failed;
            }
            break;
        case 'm':
            if (TokenMatches(cmd, cmdEnd, "mulReg")) {
                return processRegMulCommand(arg1, cmdEnd);
            }
            else {
                goto failed;
            }
            break;
        case 'n':
            if (TokenMatches(cmd, cmdEnd, "noOp")) {
                return processNoOpCommand();
            }
            else {
                goto failed;
            }
            break;
        case 'o':
            if (TokenMatches(cmd, cmdEnd, "oneShot")) {
                return processOneShotCommand(arg1, cmdEnd);
            }
            if (TokenMatches(cmd, cmdEnd, "overlayLayer")) {
                return processOverlayLayerCommand(arg1, cmdEnd);
            }
            if (TokenMatches(cmd, cmdEnd, "overlayKeymap")) {
                return processOverlayKeymapCommand(arg1, cmdEnd);
            }
            else {
                goto failed;
            }
            break;
        case 'p':
            if (TokenMatches(cmd, cmdEnd, "printStatus")) {
                return processPrintStatusCommand();
            }
            else if (TokenMatches(cmd, cmdEnd, "playMacro")) {
                return processPlayMacroCommand(arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "pressKey")) {
                return processKeyCommand(MacroSubAction_Press, arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "postponeKeys")) {
                processPostponeKeysCommand();
            }
            else if (TokenMatches(cmd, cmdEnd, "postponeNext")) {
                return processPostponeNextNCommand(arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "progressHue")) {
                return processProgressHueCommand();
            }
            else {
                goto failed;
            }
            break;
        case 'r':
            if (TokenMatches(cmd, cmdEnd, "recordMacro")) {
                return processRecordMacroCommand(arg1, cmdEnd, false);
            }
            else if (TokenMatches(cmd, cmdEnd, "recordMacroBlind")) {
                return processRecordMacroCommand(arg1, cmdEnd, true);
            }
            else if (TokenMatches(cmd, cmdEnd, "recordMacroDelay")) {
                return processRecordMacroDelayCommand();
            }
            else if (TokenMatches(cmd, cmdEnd, "resolveSecondary")) {
                return processResolveSecondaryCommand(arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "resolveNextKeyId")) {
                return processResolveNextKeyIdCommand();
            }
            else if (TokenMatches(cmd, cmdEnd, "resolveNextKeyEq")) {
                return processResolveNextKeyEqCommand(arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "releaseKey")) {
                return processKeyCommand(MacroSubAction_Release, arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "repeatFor")) {
                return processRepeatForCommand(arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "resetTrackpoint")) {
                return processResetTrackpointCommand();
            }
            else if (TokenMatches(cmd, cmdEnd, "replaceLayer")) {
                return processReplaceLayerCommand(arg1, cmdEnd);
            }
            else {
                goto failed;
            }
            break;
        case 's':
            if (TokenMatches(cmd, cmdEnd, "setStatusPart")) {
                return processSetStatusCommand(arg1, cmdEnd, false);
            }
            else if (TokenMatches(cmd, cmdEnd, "set")) {
                return MacroSetCommand(arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "setStatus")) {
                return processSetStatusCommand(arg1, cmdEnd, true);
            }
            else if (TokenMatches(cmd, cmdEnd, "startRecording")) {
                return processStartRecordingCommand(arg1, cmdEnd, false);
            }
            else if (TokenMatches(cmd, cmdEnd, "startRecordingBlind")) {
                return processStartRecordingCommand(arg1, cmdEnd, true);
            }
            else if (TokenMatches(cmd, cmdEnd, "setLedTxt")) {
                return processSetLedTxtCommand(arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "setReg")) {
                return processSetRegCommand(arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "statsRuntime")) {
                return processStatsRuntimeCommand();
            }
            else if (TokenMatches(cmd, cmdEnd, "statsLayerStack")) {
                return processStatsLayerStackCommand();
            }
            else if (TokenMatches(cmd, cmdEnd, "statsActiveKeys")) {
                return processStatsActiveKeysCommand();
            }
            else if (TokenMatches(cmd, cmdEnd, "statsActiveMacros")) {
                return processStatsActiveMacrosCommand();
            }
            else if (TokenMatches(cmd, cmdEnd, "statsRegs")) {
                return processStatsRegs();
            }
            else if (TokenMatches(cmd, cmdEnd, "statsPostponerStack")) {
                return processStatsPostponerStackCommand();
            }
            else if (TokenMatches(cmd, cmdEnd, "subReg")) {
                return processRegAddCommand(arg1, cmdEnd, true);
            }
            else if (TokenMatches(cmd, cmdEnd, "switchKeymap")) {
                return processSwitchKeymapCommand(arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "switchKeymapLayer")) {
                return processSwitchKeymapLayerCommand(arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "switchLayer")) {
                return processSwitchLayerCommand(arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "startMouse")) {
                return processMouseCommand(true, arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "stopMouse")) {
                return processMouseCommand(false, arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "stopRecording")) {
                return processStopRecordingCommand();
            }
            else if (TokenMatches(cmd, cmdEnd, "stopAllMacros")) {
                return stopAllMacrosCommand();
            }
            else if (TokenMatches(cmd, cmdEnd, "stopRecordingBlind")) {
                return processStopRecordingCommand();
            }
            else if (TokenMatches(cmd, cmdEnd, "suppressMods")) {
                processSuppressModsCommand();
            }
            else {
                goto failed;
            }
            break;
        case 't':
            if (TokenMatches(cmd, cmdEnd, "toggleKeymapLayer")) {
                return processToggleKeymapLayerCommand(arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "toggleLayer")) {
                return processToggleLayerCommand(arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "tapKey")) {
                return processKeyCommand(MacroSubAction_Tap, arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "tapKeySeq")) {
                return processTapKeySeqCommand(arg1, cmdEnd);
            }
            else {
                goto failed;
            }
            break;
        case 'u':
            if (TokenMatches(cmd, cmdEnd, "unToggleLayer")) {
                return processUnToggleLayerCommand();
            }
            else {
                goto failed;
            }
            break;
        case 'w':
            if (TokenMatches(cmd, cmdEnd, "write")) {
                return processWriteCommand(arg1, cmdEnd);
            }
            else if (TokenMatches(cmd, cmdEnd, "writeExpr")) {
                return processWriteExprCommand(arg1, cmdEnd);
            }
            else {
                goto failed;
            }
            break;
        case 'y':
            if (TokenMatches(cmd, cmdEnd, "yield")) {
                return processYieldCommand(arg1, cmdEnd);
            }
            else {
                goto failed;
            }
            break;
        default:
        failed:
            Macros_ReportError("unrecognized command", cmd, cmdEnd);
            return MacroResult_Finished;
            break;
        }
        cmd = arg1;
    }
    //this is reachable if there is a train of conditions/modifiers/labels without any command
    return MacroResult_Finished;
}

static macro_result_t processCommandAction(void)
{
    const char* cmd = s->ms.currentMacroAction.cmd.text + s->ms.commandBegin;
    const char* cmdEnd = s->ms.currentMacroAction.cmd.text + s->ms.commandEnd;

    if (cmd[0] == '#') {
        return MacroResult_Finished;
    }
    if (cmd[0] == '/' && cmd[1] == '/') {
        return MacroResult_Finished;
    }

    macro_result_t actionInProgress;
    actionInProgress = processCommand(cmd, cmdEnd);

    s->as.currentConditionPassed = actionInProgress & MacroResult_InProgressFlag;
    return actionInProgress;
}

static macro_result_t processCurrentMacroAction(void)
{
    switch (s->ms.currentMacroAction.type) {
        case MacroActionType_Delay:
            return processDelayAction();
        case MacroActionType_Key:
            return processKeyAction();
        case MacroActionType_MouseButton:
            return processMouseButtonAction();
        case MacroActionType_MoveMouse:
            return processMoveMouseAction();
        case MacroActionType_ScrollMouse:
            return processScrollMouseAction();
        case MacroActionType_Text:
            return processTextAction();
        case MacroActionType_Command:
            return processCommandAction();
    }
    return MacroResult_Finished;
}


static bool findFreeStateSlot()
{
    for (uint8_t i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
        if (!MacroState[i].ms.macroPlaying) {
            s = &MacroState[i];
            return true;
        }
    }
    Macros_ReportError("Too many macros running at one time", "", NULL);
    return false;
}


void Macros_Initialize() {
    LayerStack_Reset();
}

static uint8_t currentActionCmdCount() {
    if(s->ms.currentMacroAction.type == MacroActionType_Command) {
        return s->ms.currentMacroAction.cmd.cmdCount;
    } else {
        return 1;
    }
}

static macro_result_t endMacro(void)
{
    s->ms.macroSleeping = false;
    s->ms.macroPlaying = false;
    s->ms.macroBroken = false;
    s->ps.previousMacroIndex = s->ms.currentMacroIndex;
    s->ps.previousMacroStartTime = s->ms.currentMacroStartTime;
    unscheduleCurrentSlot();
    if (s->ms.parentMacroSlot != 255) {
        //resume our calee, if this macro was called by another macro
        wakeMacroInSlot(s->ms.parentMacroSlot);
        return MacroResult_YieldFlag;
    }
    return MacroResult_YieldFlag;
}

static void loadAction()
{
    if (s->ms.currentMacroIndex == MacroIndex_UsbCmdReserved) {
        // fill in action from memory
        s->ms.currentMacroAction = (macro_action_t){
            .type = MacroActionType_Command,
            .cmd = {
                .text = UsbMacroCommand,
                .textLen = UsbMacroCommandLength,
                .cmdCount = CountCommands(UsbMacroCommand, UsbMacroCommandLength)
            }
        };
    } else {
        // parse one macro action
        ValidatedUserConfigBuffer.offset = s->ms.bufferOffset;
        ParseMacroAction(&ValidatedUserConfigBuffer, &s->ms.currentMacroAction);
        s->ms.bufferOffset = ValidatedUserConfigBuffer.offset;
    }

    memset(&s->as, 0, sizeof s->as);

    if (s->ms.currentMacroAction.type == MacroActionType_Command) {
        const char* cmd = s->ms.currentMacroAction.cmd.text;
        const char* actionEnd = s->ms.currentMacroAction.cmd.text + s->ms.currentMacroAction.cmd.textLen;
        while ( *cmd <= 32 && cmd < actionEnd) {
            cmd++;
        }
        s->ms.commandBegin = cmd - s->ms.currentMacroAction.cmd.text;
        s->ms.commandEnd = NextCmd(cmd, actionEnd) - s->ms.currentMacroAction.cmd.text;
    }
}

static bool loadNextAction()
{
    if (s->ms.currentMacroActionIndex + 1 >= AllMacros[s->ms.currentMacroIndex].macroActionsCount) {
        return false;
    } else {
        loadAction();
        s->ms.currentMacroActionIndex++;
        s->ms.commandAddress++;
        return true;
    }
}

static bool loadNextCommand()
{
    if (s->ms.currentMacroAction.type != MacroActionType_Command) {
        return false;
    }

    memset(&s->as, 0, sizeof s->as);

    const char* actionEnd = s->ms.currentMacroAction.cmd.text + s->ms.currentMacroAction.cmd.textLen;
    const char* nextCommand = s->ms.currentMacroAction.cmd.text + s->ms.commandEnd;

    if (nextCommand == actionEnd) {
        return false;
    } else {
        s->ms.commandAddress++;
        s->ms.commandBegin = nextCommand - s->ms.currentMacroAction.cmd.text;
        s->ms.commandEnd = NextCmd(nextCommand, actionEnd) - s->ms.currentMacroAction.cmd.text;
        return true;
    }
}

static void resetToAddressZero(uint8_t macroIndex)
{
    s->ms.currentMacroIndex = macroIndex;
    s->ms.currentMacroActionIndex = 0;
    s->ms.commandAddress = 0;
    s->ms.bufferOffset = AllMacros[macroIndex].firstMacroActionOffset; //set offset to first action
    loadAction();  //loads first action, sets offset to second action
}

static bool macroIsValid(uint8_t index)
{
    bool isNonEmpty = AllMacros[index].macroActionsCount != 0;
    bool exists = index != MacroIndex_None;
    return exists && isNonEmpty;
}

static macro_result_t execMacro(uint8_t macroIndex)
{
    if (!macroIsValid(macroIndex))  {
       s->ms.macroBroken = true;
       return MacroResult_Finished;
    }

    //reset to address zero and load first address
    resetToAddressZero(macroIndex);

    if (Macros_Scheduler == Scheduler_Preemptive) {
        continueMacro();
    }

    return MacroResult_JumpedForward;
}

static macro_result_t callMacro(uint8_t macroIndex)
{
    uint32_t parentSlotIndex = s - MacroState;
    uint8_t childSlotIndex = Macros_StartMacro(macroIndex, s->ms.currentMacroKey, parentSlotIndex, true);

    if (childSlotIndex != 255) {
        unscheduleCurrentSlot();
        s->ms.macroSleeping = true;
        s->ms.wakeMeOnKeystateChange = false;
        s->ms.wakeMeOnTime = false;
    }

    return MacroResult_Finished | MacroResult_YieldFlag;
}

static macro_result_t forkMacro(uint8_t macroIndex)
{
    Macros_StartMacro(macroIndex, s->ms.currentMacroKey, 255, true);
    return MacroResult_Finished;
}

uint8_t initMacro(uint8_t index, key_state_t *keyState, uint8_t parentMacroSlot)
{
    if (!macroIsValid(index) || !findFreeStateSlot())  {
       return 255;
    }

    MacroPlaying = true;

    memset(&s->ms, 0, sizeof s->ms);

    s->ms.macroPlaying = true;
    s->ms.currentMacroIndex = index;
    s->ms.currentMacroKey = keyState;
    s->ms.currentMacroStartTime = CurrentTime;
    s->ms.parentMacroSlot = parentMacroSlot;

    //this loads the first action and resets all adresses
    resetToAddressZero(index);

    return s - MacroState;
}


//partentMacroSlot == 255 means no parent
uint8_t Macros_StartMacro(uint8_t index, key_state_t *keyState, uint8_t parentMacroSlot, bool runFirstAction)
{
    macro_state_t* oldState = s;

    uint8_t slotIndex = initMacro(index, keyState, parentMacroSlot);

    if (slotIndex == 255) {
        s = oldState;
        return slotIndex;
    }

    if (Macros_Scheduler == Scheduler_Preemptive && runFirstAction && (parentMacroSlot == 255 || s < &MacroState[parentMacroSlot])) {
        //execute first action if macro has no caller Or is being called and its caller has higher slot index.
        //The condition ensures that a called macro executes exactly one action in the same eventloop cycle.
        continueMacro();
    }

    scheduleSlot(slotIndex);
    if (Macros_Scheduler == Scheduler_Blocking) {
        // We don't care. Let it execute in regular macro execution loop, irrespectively whether this cycle or next.
        PostponerCore_PostponeNCycles(0);
    }

    s = oldState;
    return slotIndex;
}

uint8_t Macros_QueueMacro(uint8_t index, key_state_t *keyState, uint8_t queueAfterSlot)
{
    macro_state_t* oldState = s;

    uint8_t slotIndex = initMacro(index, keyState, 255);

    if (slotIndex == 255) {
        return slotIndex;
    }

    uint8_t childSlotIndex = queueAfterSlot;

    while (MacroState[childSlotIndex].ms.parentMacroSlot != 255) {
        childSlotIndex = MacroState[childSlotIndex].ms.parentMacroSlot;
    }

    MacroState[childSlotIndex].ms.parentMacroSlot = slotIndex;
    s->ms.macroSleeping = true;

    s = oldState;
    return slotIndex;
}

macro_result_t continueMacro(void)
{
    Macros_ParserError = false;
    s->as.modifierPostpone = false;
    s->as.modifierSuppressMods = false;

    if (s->ms.postponeNextNCommands > 0) {
        s->as.modifierPostpone = true;
        PostponerCore_PostponeNCycles(1);
    }

    macro_result_t res = MacroResult_YieldFlag;

    if (!s->ms.macroBroken && ((res = processCurrentMacroAction()) & (MacroResult_InProgressFlag | MacroResult_DoneFlag)) && !s->ms.macroBroken) {
        // InProgressFlag means that action is still in progress
        // DoneFlag means that someone has already done epilogue and therefore we should just return the value.
        return res;
    }

    //at this point, current action/command has finished
    s->ms.postponeNextNCommands = s->ms.postponeNextNCommands > 0 ? s->ms.postponeNextNCommands - 1 : 0;

    if ((s->ms.macroBroken) || (!loadNextCommand() && !loadNextAction())) {
        //macro was ended either because it was broken or because we are out of actions to perform.
        return endMacro() | res;
    } else {
        //we are still running - return last action's return value
        return res;
    }
}

static macro_result_t sleepTillKeystateChange()
{
    if(s->ms.oneShotState > 1) {
        return MacroResult_Blocking;
    }
    if (!s->ms.macroSleeping) {
        unscheduleCurrentSlot();
    }
    Macros_WakeMeOnKeystateChange = true;
    s->ms.wakeMeOnKeystateChange = true;
    s->ms.macroSleeping = true;
    return MacroResult_Sleeping;
}

static macro_result_t sleepTillTime(uint32_t time)
{
    if(s->ms.oneShotState > 1) {
        return MacroResult_Blocking;
    }
    if (!s->ms.macroSleeping) {
        unscheduleCurrentSlot();
    }
    EventScheduler_Schedule(time, EventSchedulerEvent_MacroWakeOnTime);
    s->ms.wakeMeOnTime = true;
    s->ms.macroSleeping = true;
    return MacroResult_Sleeping;
}

static void wakeSleepers()
{
    if (Macros_WakedBecauseOfKeystateChange) {
        Macros_WakedBecauseOfKeystateChange = false;
        Macros_WakeMeOnKeystateChange = false;
        for (uint8_t i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
            if (MacroState[i].ms.wakeMeOnKeystateChange) {
                wakeMacroInSlot(i);
            }
        }
    }
    if (Macros_WakedBecauseOfTime) {
        Macros_WakedBecauseOfTime = false;
        Macros_WakeMeOnTime = 0xFFFFFFFF;
        for (uint8_t i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
            if (MacroState[i].ms.wakeMeOnTime) {
                wakeMacroInSlot(i);
            }
        }
    }
}

static void executePreemptive(void)
{
    for (uint8_t i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
        if (MacroState[i].ms.macroPlaying && !MacroState[i].ms.macroSleeping) {
            s = &MacroState[i];

            macro_result_t res = MacroResult_Finished;
            uint8_t remainingExecution = Macros_MaxBatchSize;
            while (MacroState[i].ms.macroPlaying && !MacroState[i].ms.macroSleeping && res == MacroResult_Finished && remainingExecution > 0) {
                res = continueMacro();
                remainingExecution --;
            }
        }
    }
    s = NULL;
}

static void wakeMacroInSlot(uint8_t slotIdx)
{
    if (MacroState[slotIdx].ms.macroSleeping) {
        MacroState[slotIdx].ms.macroSleeping = false;
        MacroState[slotIdx].ms.wakeMeOnTime = false;
        MacroState[slotIdx].ms.wakeMeOnKeystateChange = false;
        scheduleSlot(slotIdx);
    }
}

static void __attribute__((__unused__)) checkSchedulerHealth(const char* tag) {
    static const char* lastCmd = NULL;
    uint8_t scheduledCount = 0;
    uint8_t playingCount = 0;

    // count active macros
    for (uint8_t i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
        if (MacroState[i].ms.macroPlaying && !MacroState[i].ms.macroSleeping) {
            playingCount++;
        }
    }

    // count length of the scheduling loop
    if (scheduler.activeSlotCount != 0) {
        uint8_t currentSlot = scheduler.currentSlotIdx;
        uint8_t startedAt = currentSlot;
        for (uint8_t i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
            scheduledCount++;

            if (!MacroState[currentSlot].ms.macroPlaying) {
                Macros_ReportErrorNum("This slot is not playing, but is scheduled!", currentSlot);
            }

            if (MacroState[currentSlot].ms.macroSleeping) {
                Macros_ReportErrorNum("This slot is sleeping, but is scheduled!", currentSlot);
            }

            currentSlot = MacroState[currentSlot].ms.nextSlot;
            if (currentSlot == startedAt) {
                break;
            }
        }
    }

    const char* thisCmd = NULL;

    // retrieve currently running command if possible
    if (s != NULL && s->ms.currentMacroAction.type == MacroActionType_Command) {
        thisCmd = s->ms.currentMacroAction.cmd.text + s->ms.commandBegin;
    }

    // check the results
    if (scheduledCount != playingCount || scheduledCount != scheduler.activeSlotCount) {
        Macros_ReportError("Scheduled counts don't match up!", tag, NULL);
        Macros_ReportErrorNum("Scheduled", scheduledCount);
        Macros_ReportErrorNum("Playing", playingCount);
        Macros_ReportErrorNum("Active slot count", scheduler.activeSlotCount);
        Macros_ReportError("Prev cmd", lastCmd, lastCmd+10);
        Macros_ReportError("This cmd", thisCmd, thisCmd+10);
        processStatsActiveMacrosCommand();
    }

    lastCmd = thisCmd;
}

static void scheduleSlot(uint8_t slotIdx)
{
    if (!MacroState[slotIdx].ms.macroScheduled) {
        if (scheduler.activeSlotCount == 0) {
            MacroState[slotIdx].ms.nextSlot = slotIdx;
            MacroState[slotIdx].ms.macroScheduled = true;
            scheduler.previousSlotIdx = slotIdx;
            scheduler.currentSlotIdx = slotIdx;
            scheduler.lastQueuedSlot = slotIdx;
            scheduler.activeSlotCount++;
            scheduler.remainingCount++;
        } else {
            bool shouldInheritPrevious = scheduler.lastQueuedSlot == scheduler.previousSlotIdx;
            MacroState[slotIdx].ms.nextSlot = MacroState[scheduler.lastQueuedSlot].ms.nextSlot;
            MacroState[scheduler.lastQueuedSlot].ms.nextSlot = slotIdx;
            MacroState[slotIdx].ms.macroScheduled = true;
            scheduler.lastQueuedSlot = slotIdx;
            scheduler.activeSlotCount++;
            scheduler.remainingCount++;
            if(shouldInheritPrevious) {
                scheduler.previousSlotIdx = slotIdx;
            }
        }
    } else {
        ERR("Scheduling an already scheduled slot attempted!");
    }
}

static void unscheduleCurrentSlot()
{
    if (scheduler.currentSlotIdx == (s - MacroState) && MacroState[scheduler.currentSlotIdx].ms.macroScheduled) {
        MacroState[scheduler.previousSlotIdx].ms.nextSlot = MacroState[scheduler.currentSlotIdx].ms.nextSlot;
        MacroState[scheduler.currentSlotIdx].ms.macroScheduled = false;
        scheduler.lastQueuedSlot = scheduler.lastQueuedSlot == scheduler.currentSlotIdx ? scheduler.previousSlotIdx : scheduler.lastQueuedSlot;
        scheduler.currentSlotIdx = scheduler.previousSlotIdx;
        scheduler.activeSlotCount--;
    } else if (scheduler.currentSlotIdx != (s - MacroState) && !s->ms.macroScheduled) {
        // This means that current slot is already unscheduled but scheduler state is fine.
        // This may happen (for instance) if callMacro is the last command of a macro.
        // (We get one unschedule attempt for sleeping the macro, but at the same time for macro end.)
        return;
    } else {
        // this means that the state is inconsistent for some reason
        ERR("Unsechuling non-scheduled slot attempted!");
    }
}

static void getNextScheduledSlot()
{
    if (scheduler.remainingCount > 0) {
        uint8_t nextSlot = MacroState[scheduler.currentSlotIdx].ms.nextSlot;
        scheduler.previousSlotIdx = scheduler.currentSlotIdx;
        scheduler.lastQueuedSlot = scheduler.lastQueuedSlot == scheduler.currentSlotIdx ? nextSlot : scheduler.lastQueuedSlot;
        scheduler.currentSlotIdx = nextSlot;
        scheduler.remainingCount--;
    }
}

static void executeBlocking(void)
{
    bool someoneBlocking = false;
    uint8_t remainingExecution = Macros_MaxBatchSize;
    scheduler.remainingCount = scheduler.activeSlotCount;

    while (scheduler.remainingCount > 0 && remainingExecution > 0) {
        macro_result_t res = MacroResult_YieldFlag;
        s = &MacroState[scheduler.currentSlotIdx];

        if (s->ms.macroPlaying && !s->ms.macroSleeping) {
            IF_DEBUG(checkSchedulerHealth("co1"));
            res = continueMacro();
            IF_DEBUG(checkSchedulerHealth("co2"));
        }

        if ((someoneBlocking = (res & MacroResult_BlockingFlag))) {
            break;
        }

        if ((res & MacroResult_YieldFlag) || !s->ms.macroPlaying || s->ms.macroSleeping) {
            getNextScheduledSlot();
        }

        remainingExecution--;
    }

    if(someoneBlocking || remainingExecution == 0) {
        PostponerCore_PostponeNCycles(0);
    }

    s = NULL;
}

static void applySleepingMods()
{
    bool someoneAlive = false;
    for (uint8_t i = 0; i < MACRO_STATE_POOL_SIZE; i++) {
        if (MacroState[i].ms.macroPlaying) {
            s = &MacroState[i];
            someoneAlive = true;
            if ( s->as.modifierPostpone ) {
                PostponerCore_PostponeNCycles(0);
            }
            if ( s->as.modifierSuppressMods ) {
                SuppressMods = true;
            }
        }
    }
    MacroPlaying = someoneAlive;
    s = NULL;
}

void Macros_ContinueMacro(void)
{
    wakeSleepers();

    switch (Macros_Scheduler) {
    case Scheduler_Preemptive:
        executePreemptive();
        applySleepingMods();
        break;
    case Scheduler_Blocking:
        executeBlocking();
        applySleepingMods();
        break;
    default:
        break;
    }
}

void Macros_ClearStatus(void)
{
    SegmentDisplay_DeactivateSlot(SegmentDisplaySlot_Error);
    processClearStatusCommand();
}

bool Macros_MacroHasActiveInstance(macro_index_t macroIdx)
{
    for(uint8_t j = 0; j < MACRO_STATE_POOL_SIZE; j++) {
        if(MacroState[j].ms.macroPlaying && MacroState[j].ms.currentMacroIndex == macroIdx) {
            return true;
        }
    }
    return false;
}

void Macros_ResetBasicKeyboardReports()
{
    for(uint8_t j = 0; j < MACRO_STATE_POOL_SIZE; j++) {
        memset(&MacroState[j].ms.macroBasicKeyboardReport, 0, sizeof MacroState[j].ms.macroBasicKeyboardReport);
    }
}
