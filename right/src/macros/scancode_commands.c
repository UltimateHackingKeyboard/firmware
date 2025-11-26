#include <string.h>
#include "macros/scancode_commands.h"
#include "config_parser/parse_keymap.h"
#include "str_utils.h"
#include "macros/typedefs.h"
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

#define MAX_UNICODE_LENGTH 2
typedef struct {
    uint32_t chars[MAX_UNICODE_LENGTH];
    uint8_t charCount;
} ATTR_PACKED unicode_sequence_t;

static void addBasicScancode(uint8_t scancode, macro_usb_keyboard_reports_t* reports)
{
    if (!scancode) {
        return;
    }
    if (!UsbBasicKeyboard_ContainsScancode(&reports->macroBasicKeyboardReport, scancode)) {
        UsbBasicKeyboard_AddScancode(&reports->macroBasicKeyboardReport, scancode);
    }
}

static void deleteBasicScancode(uint8_t scancode, macro_usb_keyboard_reports_t* reports)
{
    if (!scancode) {
        return;
    }
    UsbBasicKeyboard_RemoveScancode(&reports->macroBasicKeyboardReport, scancode);
}

static void addModifiers(uint8_t inputModifiers, uint8_t outputModifiers, macro_usb_keyboard_reports_t* reports)
{
    reports->inputModifierMask |= inputModifiers;
    reports->macroBasicKeyboardReport.modifiers |= outputModifiers;
}

static void deleteModifiers(uint8_t inputModifiers, uint8_t outputModifiers, macro_usb_keyboard_reports_t* reports)
{
    reports->inputModifierMask &= ~inputModifiers;
    reports->macroBasicKeyboardReport.modifiers &= ~outputModifiers;
}

static void addMediaScancode(uint16_t scancode, macro_usb_keyboard_reports_t* reports)
{
    if (!scancode) {
        return;
    }
    for (uint8_t i = 0; i < USB_MEDIA_KEYBOARD_MAX_KEYS; i++) {
        if (reports->macroMediaKeyboardReport.scancodes[i] == scancode) {
            return;
        }
    }
    for (uint8_t i = 0; i < USB_MEDIA_KEYBOARD_MAX_KEYS; i++) {
        if (!reports->macroMediaKeyboardReport.scancodes[i]) {
            reports->macroMediaKeyboardReport.scancodes[i] = scancode;
            break;
        }
    }
}

static void deleteMediaScancode(uint16_t scancode, macro_usb_keyboard_reports_t* reports)
{
    if (!scancode) {
        return;
    }
    for (uint8_t i = 0; i < USB_MEDIA_KEYBOARD_MAX_KEYS; i++) {
        if (reports->macroMediaKeyboardReport.scancodes[i] == scancode) {
            reports->macroMediaKeyboardReport.scancodes[i] = 0;
            return;
        }
    }
}

static void addSystemScancode(uint8_t scancode, macro_usb_keyboard_reports_t* reports)
{
    if (!scancode) {
        return;
    }
    UsbSystemKeyboard_AddScancode(&reports->macroSystemKeyboardReport, scancode);
}

static void deleteSystemScancode(uint8_t scancode, macro_usb_keyboard_reports_t* reports)
{
    if (!scancode) {
        return;
    }
    UsbSystemKeyboard_RemoveScancode(&reports->macroSystemKeyboardReport, scancode);
}

static void addScancode(uint16_t scancode, keystroke_type_t type, macro_usb_keyboard_reports_t* reports)
{
    switch (type) {
        case KeystrokeType_Basic:
            addBasicScancode(scancode, reports);
            break;
        case KeystrokeType_Media:
            addMediaScancode(scancode, reports);
            break;
        case KeystrokeType_System:
            addSystemScancode(scancode, reports);
            break;
    }
}

static void deleteScancode(uint16_t scancode, keystroke_type_t type, macro_usb_keyboard_reports_t* reports)
{
    switch (type) {
        case KeystrokeType_Basic:
            deleteBasicScancode(scancode, reports);
            break;
        case KeystrokeType_Media:
            deleteMediaScancode(scancode, reports);
            break;
        case KeystrokeType_System:
            deleteSystemScancode(scancode, reports);
            break;
    }
}

static bool containsScancode(uint16_t scancode, keystroke_type_t type, macro_usb_keyboard_reports_t* reports)
{
    switch (type) {
        case KeystrokeType_Basic:
            return UsbBasicKeyboard_ContainsScancode(&reports->macroBasicKeyboardReport, scancode);
        case KeystrokeType_Media:
            return UsbMediaKeyboard_ContainsScancode(&reports->macroMediaKeyboardReport, scancode);
        case KeystrokeType_System:
            return UsbSystemKeyboard_ContainsScancode(&reports->macroSystemKeyboardReport, scancode);
    }
    return false;
}

static bool shortcutMatches(macro_action_t macro_action, macro_usb_keyboard_reports_t* reports)
{
    bool matches = true;
    matches &= (macro_action.key.inputModMask & reports->inputModifierMask) == macro_action.key.inputModMask;
    matches &= (macro_action.key.outputModMask & reports->macroBasicKeyboardReport.modifiers) == macro_action.key.outputModMask;
    matches &= (macro_action.key.stickyModMask & reports->macroBasicKeyboardReport.modifiers) == macro_action.key.stickyModMask;
    matches &= macro_action.key.scancode == 0 || containsScancode(macro_action.key.scancode, macro_action.key.type, reports);
    return matches;
}

static macro_result_t processKey(macro_action_t macro_action, macro_usb_keyboard_reports_t* reports)
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
                    addModifiers(inputModMask, outputModMask, reports);
                    if (stickyModMask) {
                        ActivateStickyMods(S->ms.currentMacroKey, stickyModMask);
                    }
                    return MacroResult_Blocking;
                case 2:
                    addScancode(scancode, type, reports);
                    return MacroResult_Blocking;
                case 3:
                    if (Macros_CurrentMacroKeyIsActive() && action == MacroSubAction_Hold) {
                        S->as.actionPhase--;
                        return Macros_SleepTillKeystateChange();
                    }
                    deleteScancode(scancode, type, reports);
                    return MacroResult_Blocking;
                case 4:
                    deleteModifiers(inputModMask, outputModMask, reports);
                    return MacroResult_Blocking;
                case 5:
                    S->as.actionPhase = 0;
                    return MacroResult_Finished;
            }
            break;
        case MacroSubAction_Release:
            switch (S->as.actionPhase) {
                case 1:
                    deleteScancode(scancode, type, &Macros_PersistentReports);
                    deleteScancode(scancode, type, reports);
                    return MacroResult_Blocking;
                case 2:
                    deleteModifiers(inputModMask, outputModMask, &Macros_PersistentReports);
                    deleteModifiers(inputModMask, outputModMask, reports);
                    return MacroResult_Blocking;
                case 3:
                    S->as.actionPhase = 0;
                    return MacroResult_Finished;
            }
            break;
        case MacroSubAction_Press:
            switch (S->as.actionPhase) {
                case 1:
                    addModifiers(inputModMask, outputModMask, reports);
                    if (stickyModMask) {
                        ActivateStickyMods(S->ms.currentMacroKey, stickyModMask);
                    }
                    return MacroResult_Blocking;
                case 2:
                    addScancode(scancode, type, reports);
                    return MacroResult_Blocking;
                case 3:
                    S->as.actionPhase = 0;
                    return MacroResult_Finished;
            }
            break;
        case MacroSubAction_Toggle:
            switch (S->as.actionPhase) {
                case 1:
                    S->as.keySeqData.toggleShouldActivate = !shortcutMatches(macro_action, reports);
                    S->as.actionPhase++;
                    __attribute__((fallthrough));
                case 2:
                    if (S->as.keySeqData.toggleShouldActivate) {
                        addModifiers(inputModMask, outputModMask, reports);
                        if (stickyModMask) {
                            ActivateStickyMods(S->ms.currentMacroKey, stickyModMask);
                        }
                    } else {
                        deleteScancode(scancode, type, reports);
                    }
                    return MacroResult_Blocking;
                case 3:
                    if (S->as.keySeqData.toggleShouldActivate) {
                        addScancode(scancode, type, reports);
                    } else {
                        deleteModifiers(inputModMask, outputModMask, reports);
                    }
                    return MacroResult_Blocking;
                case 4:
                    S->as.actionPhase = 0;
                    S->as.keySeqData.toggleShouldActivate = false;
                    return MacroResult_Finished;
            }
            break;
    }
    return MacroResult_Finished;
}

macro_result_t Macros_ProcessKeyAction()
{
    return processKey(S->ms.currentMacroAction, &S->ms.reports);
}

static macro_result_t processMouseButton(macro_action_t macro_action, macro_usb_keyboard_reports_t* reports)
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
                reports->macroMouseReport.buttons |= mouseButtonMask;
                return MacroResult_Blocking;
            case 2:
                if (Macros_CurrentMacroKeyIsActive() && action == MacroSubAction_Hold) {
                    S->as.actionPhase--;
                    return Macros_SleepTillKeystateChange();
                }
                reports->macroMouseReport.buttons &= ~mouseButtonMask;
                return MacroResult_Blocking;
            case 3:
                S->as.actionPhase = 0;
                return MacroResult_Finished;

            }
            break;
        case MacroSubAction_Release:
            switch(S->as.actionPhase) {
            case 1:
                reports->macroMouseReport.buttons &= ~mouseButtonMask;
                return MacroResult_Blocking;
            case 2:
                S->as.actionPhase = 0;
                return MacroResult_Finished;
            }
            break;
        case MacroSubAction_Press:
            switch(S->as.actionPhase) {
                case 1:
                    reports->macroMouseReport.buttons |= mouseButtonMask;
                    return MacroResult_Blocking;
                case 2:
                    S->as.actionPhase = 0;
                    return MacroResult_Finished;
            }
            break;
        case MacroSubAction_Toggle:
            switch(S->as.actionPhase) {
                case 2:
                    reports->macroMouseReport.buttons ^= mouseButtonMask;
                    return MacroResult_Blocking;
                case 3:
                    S->as.actionPhase = 0;
                    return MacroResult_Finished;
            }
            break;
    }
    return MacroResult_Finished;
}

macro_result_t Macros_ProcessMouseButtonAction(void)
{
    return processMouseButton(S->ms.currentMacroAction, &S->ms.reports);
}

macro_result_t Macros_ProcessMoveMouseAction(void)
{
    macro_usb_keyboard_reports_t* reports = &S->ms.reports;
    S->ms.reportsUsed = true;
    if (S->as.actionActive) {
        reports->macroMouseReport.x = 0;
        reports->macroMouseReport.y = 0;
        S->as.actionActive = false;
    } else {
        reports->macroMouseReport.x = S->ms.currentMacroAction.moveMouse.x;
        reports->macroMouseReport.y = S->ms.currentMacroAction.moveMouse.y;
        S->as.actionActive = true;
    }
    return S->as.actionActive ? MacroResult_Blocking : MacroResult_Finished;
}

macro_result_t Macros_ProcessScrollMouseAction(void)
{
    macro_usb_keyboard_reports_t* reports = &S->ms.reports;
    S->ms.reportsUsed = true;
    if (S->as.actionActive) {
        reports->macroMouseReport.wheelX = 0;
        reports->macroMouseReport.wheelY = 0;
        S->as.actionActive = false;
    } else {
        reports->macroMouseReport.wheelX = S->ms.currentMacroAction.scrollMouse.x;
        reports->macroMouseReport.wheelY = S->ms.currentMacroAction.scrollMouse.y;
        S->as.actionActive = true;
    }
    return S->as.actionActive ? MacroResult_Blocking : MacroResult_Finished;
}


static void clearScancodes()
{
    uint8_t oldMods = S->ms.reports.macroBasicKeyboardReport.modifiers;
    memset(&S->ms.reports.macroBasicKeyboardReport, 0, sizeof S->ms.reports.macroBasicKeyboardReport);
    S->ms.reports.macroBasicKeyboardReport.modifiers = oldMods;
}

macro_result_t Macros_DispatchText(const char* text, uint16_t textLen, parser_context_t* ctx)
{
    const uint8_t maxGroupSize=3;
    static uint8_t currentReportSize=0;
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
        if (ctx == NULL) {
            character = text[S->as.dispatchData.textIdx];
        } else {

            character = Macros_ConsumeCharOfString(ctx, &stringOffsetCopy, &textIndexCopy, &textSubIndexCopy);

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
    if (mods != S->ms.reports.macroBasicKeyboardReport.modifiers) {
        if (S->as.dispatchData.reportState != REPORT_EMPTY) {
            S->as.dispatchData.reportState = REPORT_EMPTY;
            clearScancodes();
            currentReportSize = 0;
            return MacroResult_Blocking;
        } else {
            S->ms.reports.macroBasicKeyboardReport.modifiers = mods;
            return MacroResult_Blocking;
        }
    }

    // If all characters have been sent, finish.
    if (S->as.dispatchData.textIdx == textLen) {
        if (S->as.dispatchData.reportState != REPORT_EMPTY) {
            currentReportSize = 0;
            S->as.dispatchData.reportState = REPORT_EMPTY;
            memset(&S->ms.reports.macroBasicKeyboardReport, 0, sizeof S->ms.reports.macroBasicKeyboardReport);
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
        currentReportSize = 0;
        S->as.dispatchData.reportState = REPORT_EMPTY;
        memset(&S->ms.reports.macroBasicKeyboardReport, 0, sizeof S->ms.reports.macroBasicKeyboardReport);
        return MacroResult_Blocking;
    }

    // If current character is already contained in the report, we need to
    // release it first. We do so by artificially marking the report
    // full. Next call will do rest of the work for us.
    if (UsbBasicKeyboard_ContainsScancode(&S->ms.reports.macroBasicKeyboardReport, scancode)) {
        S->as.dispatchData.reportState = REPORT_FULL;
        return MacroResult_Blocking;
    }

    // Send the scancode.
    UsbBasicKeyboard_AddScancode(&S->ms.reports.macroBasicKeyboardReport, scancode);
    S->as.dispatchData.reportState = ++currentReportSize >= maxGroupSize ? REPORT_FULL : REPORT_PARTIAL;
    if (ctx == NULL) {
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
    return Macros_DispatchText(S->ms.currentMacroAction.text.text, S->ms.currentMacroAction.text.textLen, NULL);
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

macro_result_t Macros_ProcessKeyCommandAndConsume(parser_context_t* ctx, macro_sub_action_t type, macro_usb_keyboard_reports_t* reports)
{
    if (reports == NULL) {
        reports = &S->ms.reports;
    }
    if (ConsumeToken(ctx, "persistent") || type == MacroSubAction_Toggle) {
        reports = &Macros_PersistentReports;
    }

    macro_action_t action = decodeKeyAndConsume(ctx, type);

    if (Macros_DryRun) {
        return MacroResult_Finished;
    }

    switch (action.type) {
        case MacroActionType_Key:
            return processKey(action, reports);
        case MacroActionType_MouseButton:
            return processMouseButton(action, reports);
        default:
            return MacroResult_Finished;
    }
}

uint32_t consumeUtf8(parser_context_t* ctx) {
    char c = ctx->at[0];
    uint32_t unicode;
    uint8_t len = 1;

    if ((c & 0b10000000) == 0) {
        unicode = c;
        len = 1;
    }
    else if ((c & 0b11100000) == 0b11000000) {
        unicode = ((c & 0b00011111) << 6) | (ctx->at[1] & 0b00111111);
        len = 2;
    }
    else if ((c & 0b11110000) == 0b11100000) {
        unicode = ((c & 0b00001111) << 12) | ((ctx->at[1] & 0b00111111) << 6) | (ctx->at[2] & 0b00111111);
        len = 3;
    }
    else if ((c & 0b11111000) == 0b11110000) {
        unicode = ((c & 0b00000111) << 18) | ((ctx->at[1] & 0b00111111) << 12) | ((ctx->at[2] & 0b00111111) << 6) | (ctx->at[3] & 0b00111111);
        len = 4;
    }
    else {
        unicode = (uint32_t)'*';
        len = 1;
    }

    ctx->at += len;
    return unicode;
}

static uint8_t putStringFromEnd(char* buffer, uint8_t startIdx, const char* str, uint8_t strLen) {
    for (uint8_t i = 0; i < strLen; i++) {
        if (startIdx == 0) {
            return 0;
        }
        buffer[startIdx] = str[strLen - i - 1];
        startIdx--;
    }
    return startIdx;
}

static const char* expandCodeInBase(const char* prefix, unicode_sequence_t sequence, uint8_t base, const char* suffix, bool useNumpad) {
#define MAX_CODE_LEN ((4*10+1+11)*MAX_UNICODE_LENGTH)
    const char digits[] = "0123456789abcdef";
    static char returnBuffer[MAX_CODE_LEN];

    returnBuffer[MAX_CODE_LEN-1] = '\0';
    uint8_t startIdx = MAX_CODE_LEN - 2;
    uint8_t charWidth = useNumpad ? 4 : 2;

    uint8_t prefixLen = strlen(prefix);
    uint8_t suffixLen = strlen(suffix);

    for (uint8_t i = sequence.charCount-1; i < MAX_CODE_LEN; i--) {
        uint32_t unicodeValue = sequence.chars[i];

        startIdx = putStringFromEnd(returnBuffer, startIdx, suffix, suffixLen);
        startIdx = putStringFromEnd(returnBuffer, startIdx, " ", 1);

        while (unicodeValue != 0 && startIdx >= charWidth) {
            uint8_t digit = unicodeValue % base;
            unicodeValue /= base;

            if (useNumpad) {
                returnBuffer[startIdx] = digits[digit];
                returnBuffer[startIdx-1] = 'p';
                returnBuffer[startIdx-2] = 'n';
                returnBuffer[startIdx-3] = ' ';
            } else {
                returnBuffer[startIdx] = digits[digit];
                returnBuffer[startIdx-1] = ' ';
            }
            startIdx -= charWidth;
        }

        startIdx = putStringFromEnd(returnBuffer, startIdx, prefix, prefixLen);
        startIdx = putStringFromEnd(returnBuffer, startIdx, " ", 1);
    }

    startIdx += 1;
    return &returnBuffer[startIdx];
#undef MAX_CODE_LEN
}

static unicode_sequence_t consumeUtf8InParentheses(parser_context_t* ctx) {
    unicode_sequence_t sequence = { .charCount = 0 };

    if (!ConsumeToken(ctx, "("))
    {
        Macros_ReportError("Expected parameter list.", ctx->at, ctx->at);
        return sequence;
    }

    while (sequence.charCount < MAX_UNICODE_LENGTH && *ctx->at != ')') {
        sequence.chars[sequence.charCount++] = consumeUtf8(ctx);
        ConsumeWhite(ctx);
    }

    if (!ConsumeToken(ctx, ")"))
    {
        ConsumeAnyChar(ctx);
        Macros_ReportError("Expected closing parenthess at the end of parameter list.", ctx->at, ctx->at);
    }

    return sequence;
}

static void expandCodes(parser_context_t* ctx) {
    switch (*ctx->at) {
        case 'a':
            if (ConsumeToken(ctx, "altCodeOf")) {
                unicode_sequence_t code = consumeUtf8InParentheses(ctx);
                const char* keySeq = expandCodeInBase("pLA", code, 10, "rLA", true);
                PushParserContext(ctx, keySeq, keySeq, keySeq + strlen(keySeq));
            }
            break;
        case 'd':
            if (ConsumeToken(ctx, "decCodeOf")) {
                unicode_sequence_t code = consumeUtf8InParentheses(ctx);
                const char* keySeq = expandCodeInBase("", code, 10, "", false);
                PushParserContext(ctx, keySeq, keySeq, keySeq + strlen(keySeq));
            }
            break;
        case 'h':
            if (ConsumeToken(ctx, "hexCodeOf")) {
                unicode_sequence_t code = consumeUtf8InParentheses(ctx);
                const char* keySeq = expandCodeInBase("", code, 16, "", false);
                PushParserContext(ctx, keySeq, keySeq, keySeq + strlen(keySeq));
            }
            break;
        case 'u':
            if (ConsumeToken(ctx, "uCodeOf")) {
                unicode_sequence_t code = consumeUtf8InParentheses(ctx);
                const char* keySeq = expandCodeInBase("CS-u", code, 16, "space", false);
                PushParserContext(ctx, keySeq, keySeq, keySeq + strlen(keySeq));
            }
            break;
        default:
            break;
    }
}

macro_result_t Macros_ProcessTapKeySeqCommand(parser_context_t* ctx)
{
    macro_usb_keyboard_reports_t* reports = &S->ms.reports;
    if (ConsumeToken(ctx, "persistent")) {
        reports = &Macros_PersistentReports;
    }

    if (Macros_DryRun) {
        while(ctx->at != ctx->end) {
            expandCodes(ctx);
            Macros_ProcessKeyCommandAndConsume(ctx, MacroSubAction_Tap, reports);
        }
        return MacroResult_Finished;
    }

    for(uint8_t i = 0; i < S->as.keySeqData.atKeyIdx; i++) {
        expandCodes(ctx);
        ConsumeAnyToken(ctx);

        if(ctx->at == ctx->end) {
            S->as.keySeqData.atKeyIdx = 0;
            return MacroResult_Finished;
        };
    }

    expandCodes(ctx);
    macro_result_t res = Macros_ProcessKeyCommandAndConsume(ctx, MacroSubAction_Tap, reports);

    if(res == MacroResult_Finished) {
        S->as.keySeqData.atKeyIdx++;
    }

    return res == MacroResult_Waiting ? MacroResult_Waiting : MacroResult_Blocking;
}
