#include "default_layout.h"

#define S(k) {.type = UHK_KEY_SIMPLE, .simple = { .key = HID_KEYBOARD_SC_ ## k }}
#define L(l) {.type = UHK_KEY_LAYER, .layer = { .target = LAYER_ID_ ## l }}
#define TRNS {.type = UHK_KEY_TRANSPARENT}

#define Key_NoKey { .raw = 0 }

KEYBOARD_LAYOUT(defaultKeyboardLayout)= {
    // Layer 0
    {
        // Right
        {
            S(7_AND_AMPERSAND), S(8_AND_ASTERISK), S(9_AND_OPENING_PARENTHESIS), S(0_AND_CLOSING_PARENTHESIS), S(MINUS_AND_UNDERSCORE), S(EQUAL_AND_PLUS), S(BACKSPACE),
            S(U), S(I), S(O), S(P), S(OPENING_BRACKET_AND_OPENING_BRACE), S(CLOSING_BRACKET_AND_CLOSING_BRACE), S(BACKSLASH_AND_PIPE), S(Y),
            S(J), S(K), S(L), S(SEMICOLON_AND_COLON), S(APOSTROPHE_AND_QUOTE), S(ENTER), S(H),
            S(N), S(M), S(COMMA_AND_LESS_THAN_SIGN), S(DOT_AND_GREATER_THAN_SIGN), S(SLASH_AND_QUESTION_MARK), S(RIGHT_SHIFT), Key_NoKey,
            S(SPACE), L(MOD), L(FN), S(RIGHT_ALT), S(RIGHT_GUI), S(RIGHT_CONTROL),
        },

        // Left
        {
            S(GRAVE_ACCENT_AND_TILDE), S(1_AND_EXCLAMATION), S(2_AND_AT), S(3_AND_HASHMARK), S(4_AND_DOLLAR), S(5_AND_PERCENTAGE), S(6_AND_CARET),
            S(TAB), S(Q), S(W), S(E), S(R), Key_NoKey, S(T),
            S(CAPS_LOCK), S(A), S(S), S(D), S(F), Key_NoKey, S(G),
            S(LEFT_SHIFT), S(NON_US_BACKSLASH_AND_PIPE), S(Z), S(X), S(C), S(V), S(B),
            S(LEFT_CONTROL), S(LEFT_GUI), S(LEFT_ALT), L(FN), S(SPACE), L(MOD), Key_NoKey,
        }
    },

    // Layer 1: MOD
    {
        // Right
        {
            S(F7), S(F8), S(F9), S(F10), S(F11), S(F12), S(DELETE),
            Key_NoKey, S(UP_ARROW), Key_NoKey, S(PRINT_SCREEN), S(SCROLL_LOCK), S(PAUSE), Key_NoKey, S(PAGE_UP),
            S(LEFT_ARROW), S(DOWN_ARROW), S(RIGHT_ARROW), Key_NoKey, Key_NoKey, Key_NoKey, S(PAGE_DOWN),
            S(MEDIA_MUTE), Key_NoKey, Key_NoKey, Key_NoKey, Key_NoKey, Key_NoKey, Key_NoKey,
            Key_NoKey, TRNS, Key_NoKey, Key_NoKey, Key_NoKey,
        },

        // Left
        {
            S(ESCAPE), S(F1), S(F2), S(F3), S(F4), S(F5), S(F6),
            Key_NoKey, Key_NoKey, S(UP_ARROW), Key_NoKey, Key_NoKey, Key_NoKey, S(HOME),
            Key_NoKey, S(LEFT_ARROW), S(DOWN_ARROW), S(RIGHT_ARROW), S(DELETE), Key_NoKey, S(END),
            Key_NoKey, Key_NoKey, S(MEDIA_BACKWARD), S(MEDIA_PLAY), S(MEDIA_FORWARD), S(MEDIA_VOLUME_DOWN), S(MEDIA_VOLUME_UP),
            Key_NoKey, Key_NoKey, Key_NoKey, Key_NoKey, Key_NoKey, TRNS, Key_NoKey,
        }
    },

    // Layer 2: FN
    // Layer 3: Mouse

};

