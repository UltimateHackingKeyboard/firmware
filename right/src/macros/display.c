#include <stdio.h>
#include <string.h>
#include "macros/core.h"
#include "macros/display.h"
#include "macros/commands.h"
#include "macros/string_reader.h"
#include "macros/status_buffer.h"
#include "macros/vars.h"
#include "debug.h"
#include "stubs.h"

#ifdef __ZEPHYR__
#include "keyboard/oled/widgets/widgets.h"
#include "keyboard/oled/screens/screens.h"
#else
#include "segment_display.h"
#endif

#if DEVICE_HAS_OLED
#include "keyboard/oled/screens/screen_manager.h"
#endif

display_strings_buffs_t Macros_DisplayStringsBuffs = {
    .leftStatus = "",
    .rightStatus = "",
    .keymap = "",
    .layer = "",
    .host = "",
    .notify = "",
    .abbrev = "",
};

static uint8_t consumeDisplayString(parser_context_t* ctx, char* str, uint8_t len)
{
    uint8_t textLen = 0;

    if (Macros_IsNUM(ctx)) {
#ifndef __ZEPHYR__
        macro_variable_t value = Macros_ConsumeAnyValue(ctx);
        SegmentDisplay_SerializeVar(str, value);
        textLen = 3;
#else
        Macros_SerializeVar(str, len, Macros_ConsumeAnyValue(ctx));
#endif
        return textLen;
    } else if (ctx->at != ctx->end) {
        uint16_t stringOffset = 0, textIndex = 0, textSubIndex = 0;
        for (uint8_t i = 0; true; i++) {
            char c = Macros_ConsumeCharOfString(ctx, &stringOffset, &textIndex, &textSubIndex);
            if (c == '\0') {
                break;
            }
            if (i < len) {
                str[i] = c;
                textLen++;
            }
        }
        ConsumeWhite(ctx);

        str[textLen] = '\0';

        return textLen;
    } else {
        return 0;
    }
}

typedef enum {
    DisplayStringSlot_Unknown,
    DisplayStringSlot_Notify,
    DisplayStringSlot_LeftStatus,
    DisplayStringSlot_RightStatus,
    DisplayStringSlot_Keymap,
    DisplayStringSlot_Layer,
    DisplayStringSlot_Host,
    DisplayStringSlot_Abbrev,
} display_string_slot_t;

static void getDisplayStringBuffer(display_string_slot_t inSlot, char** outBuffer, uint8_t* outLen) {
    switch (inSlot) {
        case DisplayStringSlot_Notify:
            *outBuffer = Macros_DisplayStringsBuffs.notify;
            *outLen = sizeof(Macros_DisplayStringsBuffs.notify);
            break;
        case DisplayStringSlot_LeftStatus:
            *outBuffer = Macros_DisplayStringsBuffs.leftStatus;
            *outLen = sizeof(Macros_DisplayStringsBuffs.leftStatus);
            break;
        case DisplayStringSlot_RightStatus:
            *outBuffer = Macros_DisplayStringsBuffs.rightStatus;
            *outLen = sizeof(Macros_DisplayStringsBuffs.rightStatus);
            break;
        case DisplayStringSlot_Keymap:
            *outBuffer = Macros_DisplayStringsBuffs.keymap;
            *outLen = sizeof(Macros_DisplayStringsBuffs.keymap);
            break;
        case DisplayStringSlot_Layer:
            *outBuffer = Macros_DisplayStringsBuffs.layer;
            *outLen = sizeof(Macros_DisplayStringsBuffs.layer);
            break;
        case DisplayStringSlot_Host:
            *outBuffer = Macros_DisplayStringsBuffs.host;
            *outLen = sizeof(Macros_DisplayStringsBuffs.host);
            break;
        case DisplayStringSlot_Abbrev:
            *outBuffer = Macros_DisplayStringsBuffs.abbrev;
            *outLen = sizeof(Macros_DisplayStringsBuffs.abbrev);
            break;
        default:
            break;
    }
}

static display_string_slot_t consumeDisplayStringSlotId(parser_context_t* ctx) {
    if (ConsumeToken(ctx, "notification")) {
        return DisplayStringSlot_Notify;
    }
    else if (ConsumeToken(ctx, "leftStatus")) {
        return DisplayStringSlot_LeftStatus;
    }
    else if (ConsumeToken(ctx, "rightStatus")) {
        return DisplayStringSlot_RightStatus;
    }
    else if (ConsumeToken(ctx, "keymap")) {
        return DisplayStringSlot_Keymap;
    }
    else if (ConsumeToken(ctx, "layer")) {
        return DisplayStringSlot_Layer;
    }
    else if (ConsumeToken(ctx, "host")) {
        return DisplayStringSlot_Host;
    }
    else if (ConsumeToken(ctx, "abbrev")) {
        return DisplayStringSlot_Abbrev;
    } else {
        return DisplayStringSlot_Unknown;
    }
}

static void showStringInSlot(bool show, display_string_slot_t slotId, char* text, uint8_t textLen, uint16_t time) {
#ifndef __ZEPHYR__
    if (slotId == DisplayStringSlot_Abbrev) {
        if (show) {
            SegmentDisplay_SetText(textLen, text, SegmentDisplaySlot_Macro);
        } else {
            SegmentDisplay_DeactivateSlot(SegmentDisplaySlot_Macro);
        }
    }
#else
    switch (slotId) {
        case DisplayStringSlot_Notify:
            if (show) {
                NotificationScreen_NotifyFor(text, time);
            }
            break;
        case DisplayStringSlot_LeftStatus:
            WIDGET_REFRESH(&StatusWidget);
            break;
        case DisplayStringSlot_RightStatus:
            WIDGET_REFRESH(&StatusWidget);
            break;
        case DisplayStringSlot_Keymap:
            WIDGET_REFRESH(&KeymapLayerWidget);
            break;
        case DisplayStringSlot_Layer:
            WIDGET_REFRESH(&KeymapLayerWidget);
            break;
        case DisplayStringSlot_Host:
            WIDGET_REFRESH(&TargetWidget);
            break;
        case DisplayStringSlot_Abbrev:
            break;
        default:
            break;
    }
#endif
}

void processList(parser_context_t* ctx, bool show, uint16_t time) {
    while (ctx->at != ctx->end) {
        display_string_slot_t slotId = BY_DEVICE(DisplayStringSlot_Abbrev, DisplayStringSlot_Notify);

        display_string_slot_t parsedSlotId = consumeDisplayStringSlotId(ctx);

        if (parsedSlotId != DisplayStringSlot_Unknown) {
            slotId = parsedSlotId;
        }

        char dummy[1];
        char* bufPtr = NULL;
        uint8_t bufLen = 0;

        if (Macros_DryRun) {
            bufPtr = dummy;
            bufLen = sizeof(dummy);
        } else {
            getDisplayStringBuffer(slotId, &bufPtr, &bufLen);
        }

        consumeDisplayString(ctx, bufPtr, bufLen);

        if (!show) {
            bufPtr[0] = 0;
        }

        if (!Macros_DryRun && !Macros_ParserError) {
            showStringInSlot(show, slotId, bufPtr, bufLen, time);
        }
    }
}

macro_result_t Macros_ProcessSetLedTxtCommand(parser_context_t* ctx)
{
    ATTR_UNUSED int16_t time = Macros_ConsumeInt(ctx);

    macro_result_t res = MacroResult_Finished;

    bool delayIsInProgress = S->as.actionActive;

    if (time == 0) {
        processList(ctx, true, time);
        return MacroResult_Finished;
    } else if ((res = Macros_ProcessDelay(time)) == MacroResult_Finished) {
        processList(ctx, false, time);
        return MacroResult_Finished;
    } else {
        if (!delayIsInProgress || Macros_DryRun) {
            processList(ctx, true, time);
        }
        return res;
    }
}

void NotifyPrintf(const char *fmt, ...)
{
#if DEVICE_HAS_OLED
    char* buf;
    uint8_t bufLen;

    va_list myargs;

    getDisplayStringBuffer(DisplayStringSlot_Notify, &buf, &bufLen);

    va_start(myargs, fmt);
    vsnprintf(buf, bufLen, fmt, myargs);

    buf[bufLen - 1] = '\0';

    NotificationScreen_NotifyFor(buf, SCREEN_NOTIFICATION_TIMEOUT);
#endif
}



