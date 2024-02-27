#include "macros/scancode_commands.h"
#include "usb_interfaces/usb_interface_basic_keyboard.h"
#include "usb_interfaces/usb_interface_media_keyboard.h"
#include "usb_interfaces/usb_interface_mouse.h"
#include "usb_interfaces/usb_interface_system_keyboard.h"
#include "key_action.h"
#include "macros/core.h"
#include "macros/status_buffer.h"
#include "usb_report_updater.h"
#include "macros/shortcut_parser.h"
#include "macros/string_reader.h"

static void addBasicScancode(uint8_t scancode)
{
    if (!scancode) {
        return;
    }
    if (!UsbBasicKeyboard_ContainsScancode(&S->ms.macroBasicKeyboardReport, scancode)) {
        UsbBasicKeyboard_AddScancode(&S->ms.macroBasicKeyboardReport, scancode);
    }
}

static void deleteBasicScancode(uint8_t scancode)
{
    if (!scancode) {
        return;
    }
    UsbBasicKeyboard_RemoveScancode(&S->ms.macroBasicKeyboardReport, scancode);
}

static void addModifiers(uint8_t inputModifiers, uint8_t outputModifiers)
{
    S->ms.inputModifierMask |= inputModifiers;
    S->ms.macroBasicKeyboardReport.modifiers |= outputModifiers;
}

static void deleteModifiers(uint8_t inputModifiers, uint8_t outputModifiers)
{
    S->ms.inputModifierMask &= ~inputModifiers;
    S->ms.macroBasicKeyboardReport.modifiers &= ~outputModifiers;
}

static void addMediaScancode(uint16_t scancode)
{
    if (!scancode) {
        return;
    }
    for (uint8_t i = 0; i < USB_MEDIA_KEYBOARD_MAX_KEYS; i++) {
        if (S->ms.macroMediaKeyboardReport.scancodes[i] == scancode) {
            return;
        }
    }
    for (uint8_t i = 0; i < USB_MEDIA_KEYBOARD_MAX_KEYS; i++) {
        if (!S->ms.macroMediaKeyboardReport.scancodes[i]) {
            S->ms.macroMediaKeyboardReport.scancodes[i] = scancode;
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
        if (S->ms.macroMediaKeyboardReport.scancodes[i] == scancode) {
            S->ms.macroMediaKeyboardReport.scancodes[i] = 0;
            return;
        }
    }
}

static void addSystemScancode(uint8_t scancode)
{
    if (!scancode) {
        return;
    }
    UsbSystemKeyboard_AddScancode(&S->ms.macroSystemKeyboardReport, scancode);
}

static void deleteSystemScancode(uint8_t scancode)
{
    if (!scancode) {
        return;
    }
    UsbSystemKeyboard_RemoveScancode(&S->ms.macroSystemKeyboardReport, scancode);
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

static macro_result_t processKey(macro_action_t macro_action)
{
    S->ms.reportsUsed = true;
    macro_sub_action_t action = macro_action.key.action;
    keystroke_type_t type = macro_action.key.type;
    uint8_t inputModMask = macro_action.key.inputModMask;
    uint8_t outputModMask = macro_action.key.outputModMask;
    uint8_t stickyModMask = macro_action.key.stickyModMask;
    uint16_t scancode = macro_action.key.scancode;

    S->as.actionPhase++;

    switch (action) {
        case MacroSubAction_Hold:
        case MacroSubAction_Tap:
            switch(S->as.actionPhase) {
                case 1:
                    addModifiers(inputModMask, outputModMask);
                    if (stickyModMask) {
                        ActivateStickyMods(S->ms.currentMacroKey, stickyModMask);
                    }
                    return MacroResult_Blocking;
                case 2:
                    addScancode(scancode, type);
                    return MacroResult_Blocking;
                case 3:
                    if (Macros_CurrentMacroKeyIsActive() && action == MacroSubAction_Hold) {
                        S->as.actionPhase--;
                        return Macros_SleepTillKeystateChange();
                    }
                    deleteScancode(scancode, type);
                    return MacroResult_Blocking;
                case 4:
                    deleteModifiers(inputModMask, outputModMask);
                    return MacroResult_Blocking;
                case 5:
                    S->as.actionPhase = 0;
                    return MacroResult_Finished;
            }
            break;
        case MacroSubAction_Release:
            switch (S->as.actionPhase) {
                case 1:
                    deleteScancode(scancode, type);
                    return MacroResult_Blocking;
                case 2:
                    deleteModifiers(inputModMask, outputModMask);
                    return MacroResult_Blocking;
                case 3:
                    S->as.actionPhase = 0;
                    return MacroResult_Finished;
            }
            break;
        case MacroSubAction_Press:
            switch (S->as.actionPhase) {
                case 1:
                    addModifiers(inputModMask, outputModMask);
                    if (stickyModMask) {
                        ActivateStickyMods(S->ms.currentMacroKey, stickyModMask);
                    }
                    return MacroResult_Blocking;
                case 2:
                    addScancode(scancode, type);
                    return MacroResult_Blocking;
                case 3:
                    S->as.actionPhase = 0;
                    return MacroResult_Finished;
            }
            break;
    }
    return MacroResult_Finished;
}

macro_result_t Macros_ProcessKeyAction()
{
    return processKey(S->ms.currentMacroAction);
}

static macro_result_t processMouseButton(macro_action_t macro_action)
{
    S->ms.reportsUsed = true;
    uint32_t mouseButtonMask = macro_action.mouseButton.mouseButtonsMask;
    macro_sub_action_t action = macro_action.mouseButton.action;

    S->as.actionPhase++;

    switch (macro_action.mouseButton.action) {
        case MacroSubAction_Hold:
        case MacroSubAction_Tap:
            switch(S->as.actionPhase) {
            case 1:
                S->ms.macroMouseReport.buttons |= mouseButtonMask;
                return MacroResult_Blocking;
            case 2:
                if (Macros_CurrentMacroKeyIsActive() && action == MacroSubAction_Hold) {
                    S->as.actionPhase--;
                    return Macros_SleepTillKeystateChange();
                }
                S->ms.macroMouseReport.buttons &= ~mouseButtonMask;
                return MacroResult_Blocking;
            case 3:
                S->as.actionPhase = 0;
                return MacroResult_Finished;

            }
            break;
        case MacroSubAction_Release:
            switch(S->as.actionPhase) {
            case 1:
                S->ms.macroMouseReport.buttons &= ~mouseButtonMask;
                return MacroResult_Blocking;
            case 2:
                S->as.actionPhase = 0;
                return MacroResult_Finished;
            }
            break;
        case MacroSubAction_Press:
            switch(S->as.actionPhase) {
                case 1:
                    S->ms.macroMouseReport.buttons |= mouseButtonMask;
                    return MacroResult_Blocking;
                case 2:
                    S->as.actionPhase = 0;
                    return MacroResult_Finished;
            }
            break;
    }
    return MacroResult_Finished;
}

macro_result_t Macros_ProcessMouseButtonAction(void)
{
    return processMouseButton(S->ms.currentMacroAction);
}

macro_result_t Macros_ProcessMoveMouseAction(void)
{
    S->ms.reportsUsed = true;
    if (S->as.actionActive) {
        S->ms.macroMouseReport.x = 0;
        S->ms.macroMouseReport.y = 0;
        S->as.actionActive = false;
    } else {
        S->ms.macroMouseReport.x = S->ms.currentMacroAction.moveMouse.x;
        S->ms.macroMouseReport.y = S->ms.currentMacroAction.moveMouse.y;
        S->as.actionActive = true;
    }
    return S->as.actionActive ? MacroResult_Blocking : MacroResult_Finished;
}

macro_result_t Macros_ProcessScrollMouseAction(void)
{
    S->ms.reportsUsed = true;
    if (S->as.actionActive) {
        S->ms.macroMouseReport.wheelX = 0;
        S->ms.macroMouseReport.wheelY = 0;
        S->as.actionActive = false;
    } else {
        S->ms.macroMouseReport.wheelX = S->ms.currentMacroAction.scrollMouse.x;
        S->ms.macroMouseReport.wheelY = S->ms.currentMacroAction.scrollMouse.y;
        S->as.actionActive = true;
    }
    return S->as.actionActive ? MacroResult_Blocking : MacroResult_Finished;
}


static void clearScancodes()
{
    uint8_t oldMods = S->ms.macroBasicKeyboardReport.modifiers;
    memset(&S->ms.macroBasicKeyboardReport, 0, sizeof S->ms.macroBasicKeyboardReport);
    S->ms.macroBasicKeyboardReport.modifiers = oldMods;
}

macro_result_t Macros_DispatchText(const char* text, uint16_t textLen, bool rawString)
{
    S->ms.reportsUsed = true;
    static macro_state_t* dispatchMutex = NULL;
    if (dispatchMutex != S && dispatchMutex != NULL) {
        return MacroResult_Waiting;
    } else {
        dispatchMutex = S;
    }
    char character = 0;
    uint8_t scancode = 0;
    uint8_t mods = 0;

    uint16_t stringOffsetCopy = S->as.dispatchData.stringOffset;
    uint16_t textIndexCopy = S->as.dispatchData.textIdx;
    uint16_t textSubIndexCopy = S->as.dispatchData.subIndex;

    // Precompute modifiers and scancode.
    if (S->as.dispatchData.textIdx != textLen) {
        if (rawString) {
            character = text[S->as.dispatchData.textIdx];
        } else {
            parser_context_t ctx = { .macroState = S, .begin = text, .at = text, .end = text+textLen };
            character = Macros_ConsumeCharOfString(&ctx, &stringOffsetCopy, &textIndexCopy, &textSubIndexCopy);

            //make sure we write error only once
            if (Macros_ParserError) {
                S->as.dispatchData.textIdx = textIndexCopy;
                S->as.dispatchData.subIndex = textSubIndexCopy;
                S->as.dispatchData.stringOffset = stringOffsetCopy;
                return MacroResult_InProgressFlag;
            }

            if (character == '\0') {
                S->as.dispatchData.textIdx = textLen;
            }
        }
        scancode = MacroShortcutParser_CharacterToScancode(character);
        mods = MacroShortcutParser_CharacterToShift(character) ? HID_KEYBOARD_MODIFIER_LEFTSHIFT : 0;
    }

    // If required modifiers differ, first clear scancodes and send empty report
    // containing only old modifiers. Then set new modifiers and send that new report.
    // Just then continue.
    if (mods != S->ms.macroBasicKeyboardReport.modifiers) {
        if (S->as.dispatchData.reportState != REPORT_EMPTY) {
            S->as.dispatchData.reportState = REPORT_EMPTY;
            clearScancodes();
            return MacroResult_Blocking;
        } else {
            S->ms.macroBasicKeyboardReport.modifiers = mods;
            return MacroResult_Blocking;
        }
    }

    // If all characters have been sent, finish.
    if (S->as.dispatchData.textIdx == textLen) {
        if (S->as.dispatchData.reportState != REPORT_EMPTY) {
            S->as.dispatchData.reportState = REPORT_EMPTY;
            memset(&S->ms.macroBasicKeyboardReport, 0, sizeof S->ms.macroBasicKeyboardReport);
            return MacroResult_Blocking;
        } else {
            S->as.dispatchData.textIdx = 0;
            S->as.dispatchData.subIndex = 0;
            dispatchMutex = NULL;
            return MacroResult_Finished;
        }
    }

    // Whenever the report is full, we clear the report and send it empty before continuing.
    if (S->as.dispatchData.reportState == REPORT_FULL) {
        S->as.dispatchData.reportState = REPORT_EMPTY;
        memset(&S->ms.macroBasicKeyboardReport, 0, sizeof S->ms.macroBasicKeyboardReport);
        return MacroResult_Blocking;
    }

    // If current character is already contained in the report, we need to
    // release it first. We do so by artificially marking the report
    // full. Next call will do rest of the work for us.
    if (UsbBasicKeyboard_ContainsScancode(&S->ms.macroBasicKeyboardReport, scancode)) {
        S->as.dispatchData.reportState = REPORT_FULL;
        return MacroResult_Blocking;
    }

    // Send the scancode.
    UsbBasicKeyboard_AddScancode(&S->ms.macroBasicKeyboardReport, scancode);
    S->as.dispatchData.reportState = UsbBasicKeyboard_IsFullScancodes(&S->ms.macroBasicKeyboardReport) ?
            REPORT_FULL : REPORT_PARTIAL;
    if (rawString) {
        ++S->as.dispatchData.textIdx;
    } else {
        if (textIndexCopy == S->as.dispatchData.textIdx && textSubIndexCopy == S->as.dispatchData.subIndex && stringOffsetCopy == S->as.dispatchData.stringOffset) {
            Macros_ReportError("Text dispatch got stuck! Please report this.", text + textIndexCopy, text+textLen);
            return MacroResult_Finished;
        }
        S->as.dispatchData.textIdx = textIndexCopy;
        S->as.dispatchData.subIndex = textSubIndexCopy;
        S->as.dispatchData.stringOffset = stringOffsetCopy;
    }
    return MacroResult_Blocking;
}

macro_result_t Macros_ProcessTextAction(void)
{
    return Macros_DispatchText(S->ms.currentMacroAction.text.text, S->ms.currentMacroAction.text.textLen, true);
}

static macro_action_t decodeKeyAndConsume(parser_context_t* ctx, macro_sub_action_t defaultSubAction)
{
    macro_action_t action;
    const char* end = TokEnd(ctx->at, ctx->end);
    MacroShortcutParser_Parse(ctx->at, end, defaultSubAction, &action, NULL);
    ctx->at = end;
    ConsumeWhite(ctx);

    return action;
}

macro_result_t Macros_ProcessKeyCommandAndConsume(parser_context_t* ctx, macro_sub_action_t type)
{
    macro_action_t action = decodeKeyAndConsume(ctx, type);

    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    switch (action.type) {
        case MacroActionType_Key:
            return processKey(action);
        case MacroActionType_MouseButton:
            return processMouseButton(action);
        default:
            return MacroResult_Finished;
    }
}

macro_result_t Macros_ProcessTapKeySeqCommand(parser_context_t* ctx)
{
    if (Macros_DryRun) {
        while(ctx->at != ctx->end) {
            Macros_ProcessKeyCommandAndConsume(ctx, MacroSubAction_Tap);
        }
        return MacroResult_Finished;
    }

    for(uint8_t i = 0; i < S->as.keySeqData.atKeyIdx; i++) {
        ctx->at = NextTok(ctx->at, ctx->end);

        if(ctx->at == ctx->end) {
            S->as.keySeqData.atKeyIdx = 0;
            return MacroResult_Finished;
        };
    }

    macro_result_t res = Macros_ProcessKeyCommandAndConsume(ctx, MacroSubAction_Tap);

    if(res == MacroResult_Finished) {
        S->as.keySeqData.atKeyIdx++;
    }

    return res == MacroResult_Waiting ? MacroResult_Waiting : MacroResult_Blocking;
}
