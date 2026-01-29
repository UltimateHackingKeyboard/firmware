/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: /home/karel/ncs/toolchains/b81a7cd864/usr/bin/gperf -t -L ANSI-C -N command_lookup -H command_hash /opt/west1/firmware/right/src/macros/command_hash.gperf  */
/* Computed positions: -k'2-3,6,$' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gperf@gnu.org>."
#endif

#line 1 "/opt/west1/firmware/right/src/macros/command_hash.gperf"

#include "command_ids.h"
#line 4 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
struct command_entry { const char *name; command_id_t id; };;

#define TOTAL_KEYWORDS 157
#define MIN_WORD_LENGTH 1
#define MAX_WORD_LENGTH 23
#define MIN_HASH_VALUE 1
#define MAX_HASH_VALUE 330
/* maximum key range = 330, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
command_hash (register const char *str, register size_t len)
{
  static unsigned short asso_values[] =
    {
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331,  55,  50,  85,  20,  85,
      331, 135,   0,  45, 331,  90,  60,  60,  25,  20,
       60, 331,   0,  20,  80, 145,  15, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331,   5, 125,  70,
        5,   5,  35,  15,  20,  20, 331, 145,  20,  55,
       60,   5,  45,  80,   0,  35,   0,  55,  45,  25,
      140,  50, 331,   5, 331,   0, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331, 331, 331, 331, 331,
      331, 331, 331, 331, 331, 331
    };
  register unsigned int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[5]];
      /*FALLTHROUGH*/
      case 5:
      case 4:
      case 3:
        hval += asso_values[(unsigned char)str[2]];
      /*FALLTHROUGH*/
      case 2:
        hval += asso_values[(unsigned char)str[1]];
      /*FALLTHROUGH*/
      case 1:
        break;
    }
  return hval + asso_values[(unsigned char)str[len - 1]];
}

struct command_entry *
command_lookup (register const char *str, register size_t len)
{
  static struct command_entry wordlist[] =
    {
      {""},
#line 162 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"}", CommandId_closeBrace},
      {""}, {""}, {""}, {""},
#line 161 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"{", CommandId_openBrace},
      {""},
#line 116 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"set", CommandId_set},
      {""}, {""},
#line 117 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"setVar", CommandId_setVar},
      {""}, {""}, {""},
#line 149 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"trace", CommandId_trace},
      {""}, {""}, {""},
#line 121 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"setLedTxt", CommandId_setLedTxt},
      {""},
#line 23 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"freeze", CommandId_freeze},
#line 122 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"statsRuntime", CommandId_statsRuntime},
#line 139 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"setStatusPart", CommandId_setStatusPart},
#line 132 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"stopMouse", CommandId_stopMouse},
      {""},
#line 99 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"progressHue", CommandId_progressHue},
      {""}, {""},
#line 120 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"startRecordingBlind", CommandId_startRecordingBlind},
#line 156 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"write", CommandId_write},
      {""}, {""},
#line 135 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"stopRecordingBlind", CommandId_stopRecordingBlind},
#line 119 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"startRecording", CommandId_startRecording},
#line 159 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"yield", CommandId_yield},
#line 144 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"toggleLayer", CommandId_toggleLayer},
      {""},
#line 133 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"stopRecording", CommandId_stopRecording},
#line 25 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"holdLayer", CommandId_holdLayer},
#line 123 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"statsRecordKeyTiming", CommandId_statsRecordKeyTiming},
#line 137 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"setReg", CommandId_setReg},
#line 143 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"toggleKeymapLayer", CommandId_toggleKeymapLayer},
#line 17 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"diagnose", CommandId_diagnose},
#line 95 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"playMacro", CommandId_playMacro},
#line 27 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"holdKeymapLayer", CommandId_holdKeymapLayer},
#line 8 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"addReg", CommandId_addReg},
      {""}, {""},
#line 13 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"call", CommandId_call},
#line 157 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"while", CommandId_while},
#line 77 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifHold", CommandId_ifHold},
      {""}, {""},
#line 118 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"setStatus", CommandId_setStatus},
#line 35 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifReleased", CommandId_ifReleased},
#line 160 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"zephyr", CommandId_zephyr},
#line 98 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"postponeNext", CommandId_postponeNext},
#line 63 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifRecordingId", CommandId_ifRecordingId},
#line 107 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"repeatFor", CommandId_repeatFor},
#line 148 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"trackpoint", CommandId_trackpoint},
#line 150 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"testLeakage", CommandId_testLeakage},
      {""},
#line 154 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"validateUserConfig", CommandId_validateUserConfig},
#line 19 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"else", CommandId_else},
#line 81 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifShortcut", CommandId_ifShortcut},
#line 61 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifRecording", CommandId_ifRecording},
#line 92 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"overlayLayer", CommandId_overlayLayer},
      {""},
#line 128 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"statsVariables", CommandId_statsVariables},
#line 7 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"autoRepeat", CommandId_autoRepeat},
      {""},
#line 30 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"if", CommandId_if},
#line 134 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"stopAllMacros", CommandId_stopAllMacros},
#line 90 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"noOp", CommandId_noOp},
#line 142 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"switchHost", CommandId_switchHost},
#line 141 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"switchLayer", CommandId_switchLayer},
#line 91 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"oneShot", CommandId_oneShot},
#line 36 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifNotReleased", CommandId_ifNotReleased},
#line 155 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"validateMacros", CommandId_validateMacros},
#line 131 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"startMouse", CommandId_startMouse},
#line 64 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifNotRecordingId", CommandId_ifNotRecordingId},
#line 140 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"switchKeymapLayer", CommandId_switchKeymapLayer},
      {""},
#line 147 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"toggleKey", CommandId_toggleKey},
      {""},
#line 94 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"printStatus", CommandId_printStatus},
#line 29 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"holdKey", CommandId_holdKey},
#line 152 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"untoggleLayer", CommandId_untoggleLayer},
#line 62 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifNotRecording", CommandId_ifNotRecording},
#line 48 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifNotShift", CommandId_ifNotShift},
#line 12 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"clearStatus", CommandId_clearStatus},
#line 97 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"postponeKeys", CommandId_postponeKeys},
#line 82 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifNotShortcut", CommandId_ifNotShortcut},
#line 24 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"goTo", CommandId_goTo},
#line 51 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifAlt", CommandId_ifAlt},
#line 102 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"recordMacro", CommandId_recordMacro},
#line 47 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifShift", CommandId_ifShift},
      {""}, {""},
#line 6 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"activateKeyPostponed", CommandId_activateKeyPostponed},
#line 103 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"recordMacroBlind", CommandId_recordMacroBlind},
#line 88 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifRegLt", CommandId_ifRegLt},
#line 33 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifInterrupted", CommandId_ifInterrupted},
#line 100 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"powerMode", CommandId_powerMode},
#line 21 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"final", CommandId_final},
#line 105 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"resolveNextKeyId", CommandId_resolveNextKeyId},
#line 39 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifLayer", CommandId_ifLayer},
      {""},
#line 10 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"bluetooth", CommandId_bluetooth},
#line 125 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"statsActiveKeys", CommandId_statsActiveKeys},
#line 89 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"mulReg", CommandId_mulReg},
#line 126 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"statsActiveMacros", CommandId_statsActiveMacros},
#line 93 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"overlayKeymap", CommandId_overlayKeymap},
#line 158 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"writeExpr", CommandId_writeExpr},
#line 153 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"unpairHost", CommandId_unpairHost},
      {""},
#line 129 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"statsBattery", CommandId_statsBattery},
      {""},
#line 41 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifLayerToggled", CommandId_ifLayerToggled},
#line 106 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"releaseKey", CommandId_releaseKey},
#line 79 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifSecondary", CommandId_ifSecondary},
#line 130 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"switchKeymap", CommandId_switchKeymap},
#line 52 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifNotAlt", CommandId_ifNotAlt},
#line 66 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifPending", CommandId_ifPending},
#line 71 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifPendingKeyReleased", CommandId_ifPendingKeyReleased},
#line 34 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifNotInterrupted", CommandId_ifNotInterrupted},
      {""}, {""}, {""},
#line 40 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifNotLayer", CommandId_ifNotLayer},
#line 46 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifNotAnyMod", CommandId_ifNotAnyMod},
#line 109 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"replaceLayer", CommandId_replaceLayer},
      {""},
#line 59 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifScrollLockOn", CommandId_ifScrollLockOn},
#line 108 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"resetTrackpoint", CommandId_resetTrackpoint},
#line 114 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"reboot", CommandId_reboot},
      {""},
#line 44 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifNotPlaytime", CommandId_ifNotPlaytime},
#line 32 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifNotDoubletap", CommandId_ifNotDoubletap},
#line 101 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"panic", CommandId_panic},
      {""},
#line 42 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifNotLayerToggled", CommandId_ifNotLayerToggled},
      {""},
#line 115 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"reconnect", CommandId_reconnect},
#line 76 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifNotModuleConnected", CommandId_ifNotModuleConnected},
#line 104 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"recordMacroDelay", CommandId_recordMacroDelay},
#line 65 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifNotPending", CommandId_ifNotPending},
#line 72 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifNotPendingKeyReleased", CommandId_ifNotPendingKeyReleased},
#line 11 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"consumePending", CommandId_consumePending},
#line 86 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifNotRegEq", CommandId_ifNotRegEq},
#line 112 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"resolveSecondary", CommandId_resolveSecondary},
#line 136 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"suppressMods", CommandId_suppressMods},
#line 96 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"pressKey", CommandId_pressKey},
#line 22 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"fork", CommandId_fork},
#line 9 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"break", CommandId_break},
#line 145 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"tapKey", CommandId_tapKey},
#line 60 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifNotScrollLockOn", CommandId_ifNotScrollLockOn},
      {""},
#line 58 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifNotNumLockOn", CommandId_ifNotNumLockOn},
#line 43 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifPlaytime", CommandId_ifPlaytime},
      {""},
#line 73 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifKeyDefined", CommandId_ifKeyDefined},
#line 45 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifAnyMod", CommandId_ifAnyMod},
#line 20 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"exit", CommandId_exit},
#line 78 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifTap", CommandId_ifTap},
#line 49 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifCtrl", CommandId_ifCtrl},
#line 68 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifNotKeyPendingAt", CommandId_ifNotKeyPendingAt},
#line 151 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"unToggleLayer", CommandId_unToggleLayer},
#line 70 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifNotKeyActive", CommandId_ifNotKeyActive},
#line 74 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifNotKeyDefined", CommandId_ifNotKeyDefined},
      {""},
#line 75 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifModuleConnected", CommandId_ifModuleConnected},
      {""},
#line 50 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifNotCtrl", CommandId_ifNotCtrl},
      {""}, {""},
#line 87 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifRegGt", CommandId_ifRegGt},
#line 110 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"replaceKeymap", CommandId_replaceKeymap},
      {""}, {""},
#line 111 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"resolveNextKeyEq", CommandId_resolveNextKeyEq},
#line 26 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"holdLayerMax", CommandId_holdLayerMax},
      {""},
#line 83 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifGesture", CommandId_ifGesture},
      {""}, {""}, {""},
#line 28 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"holdKeymapLayerMax", CommandId_holdKeymapLayerMax},
#line 146 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"tapKeySeq", CommandId_tapKeySeq},
      {""},
#line 57 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifNumLockOn", CommandId_ifNumLockOn},
#line 14 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"delayUntilRelease", CommandId_delayUntilRelease},
      {""}, {""},
#line 53 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifGui", CommandId_ifGui},
#line 69 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifKeyActive", CommandId_ifKeyActive},
      {""}, {""},
#line 67 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifKeyPendingAt", CommandId_ifKeyPendingAt},
#line 16 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"delayUntil", CommandId_delayUntil},
      {""}, {""},
#line 113 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"resetConfiguration", CommandId_resetConfiguration},
      {""}, {""},
#line 38 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifNotKeymap", CommandId_ifNotKeymap},
#line 85 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifRegEq", CommandId_ifRegEq},
      {""},
#line 80 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifPrimary", CommandId_ifPrimary},
      {""}, {""},
#line 84 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifNotGesture", CommandId_ifNotGesture},
      {""}, {""}, {""},
#line 138 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"subReg", CommandId_subReg},
      {""}, {""},
#line 18 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"exec", CommandId_exec},
#line 56 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifNotCapsLockOn", CommandId_ifNotCapsLockOn},
      {""}, {""},
#line 54 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifNotGui", CommandId_ifNotGui},
      {""},
#line 124 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"statsLayerStack", CommandId_statsLayerStack},
      {""},
#line 55 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifCapsLockOn", CommandId_ifCapsLockOn},
      {""},
#line 127 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"statsPostponerStack", CommandId_statsPostponerStack},
      {""}, {""}, {""},
#line 37 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifKeymap", CommandId_ifKeymap},
      {""}, {""},
#line 31 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"ifDoubletap", CommandId_ifDoubletap},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""},
#line 15 "/opt/west1/firmware/right/src/macros/command_hash.gperf"
      {"delayUntilReleaseMax", CommandId_delayUntilReleaseMax}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = command_hash (str, len);

      if (key <= MAX_HASH_VALUE)
        {
          register const char *s = wordlist[key].name;

          if (*str == *s && !strcmp (str + 1, s + 1))
            return &wordlist[key];
        }
    }
  return 0;
}
#line 163 "/opt/west1/firmware/right/src/macros/command_hash.gperf"

