# Reference manual

This file contains (semi)formal documentation of all features of the extended engine. Please note:

- You might want to start by reading [user-guide manual](user-guide.md), especially the point on understanding the docs.

- Note that by default, only `printStatus`, `resetTrackpoint` and (some portion of) `set` are available. Rest can be enabled by `set macroEngine.extendedCommands 1`, but are not officially supported, and are more likely to be unstable, or removed in the future.

- The grammar is meant to be the ultimate information source. Not all commands or parameters are described in the later text.

### Error handling

Whenever a garbled command is encountered, `ERR` will light up on the display, and details are appended to the error buffer. You can retrieve it by running a `printStatus` macro command over a focused text editor.

Logs are prefixed with macro name, action index and command address.

## Macro events

Macro events allow hooking special behaviour, such as applying specific configuration, to events. This is done via a special naming scheme. Currently, following names are supported:

    $onInit
    $onKeymapChange {KEYMAPID|any}

I.e., if you want to customize acceleration driver for your trackball module on keymap QWR, create macro named `$onKeymapChange QWR`, with content e.g.:

    set module.trackball.baseSpeed 0.5
    set module.trackball.speed 1.0
    set module.trackball.xceleration 1.0

## Macro commands

The following grammar is supported:

    BODY = #<comment>
    BODY = //<comment>
    BODY = [LABEL:] COMMAND [//<comment, excluding commands taking custom text arguments>]
    COMMAND = [CONDITION|MODIFIER]* COMMAND
    COMMAND = delayUntilRelease
    COMMAND = delayUntil <timeout (NUMBER)>
    COMMAND = delayUntilReleaseMax <timeout (NUMBER)>
    COMMAND = switchKeymap KEYMAPID
    COMMAND = toggleLayer LAYERID
    COMMAND = toggleKeymapLayer KEYMAPID LAYERID
    COMMAND = unToggleLayer
    COMMAND = holdLayer LAYERID
    COMMAND = holdLayerMax LAYERID <time in ms (NUMBER)>
    COMMAND = holdKeymapLayer KEYMAPID LAYERID
    COMMAND = holdKeymapLayerMax KEYMAPID LAYERID <time in ms (NUMBER)>
    COMMAND = resolveSecondary <time in ms (NUMBER)> [<time in ms (NUMBER)>] <primary action macro action index (ADDRESS)> <secondary action macro action index (ADDRESS)>
    COMMAND = resolveNextKeyId
    COMMAND = activateKeyPostponed [atLayer LAYERID] KEYID
    COMMAND = consumePending <number of keys (NUMBER)>
    COMMAND = postponeNext <number of commands (NUMER)>
    COMMAND = break
    COMMAND = noOp
    COMMAND = yield
    COMMAND = {exec|call|fork} MACRONAME
    COMMAND = stopAllMacros
    COMMAND = statsRuntime
    COMMAND = statsLayerStack
    COMMAND = statsPostponerStack
    COMMAND = statsActiveKeys
    COMMAND = statsActiveMacros
    COMMAND = statsRegs
    COMMAND = resetTrackpoint
    COMMAND = diagnose
    COMMAND = printStatus
    COMMAND = {setStatus  | setStatusPart} <custom text>
    COMMAND = clearStatus
    COMMAND = setLedTxt <timeout (NUMBER)> <custom text>
    COMMAND = write <custom text>
    COMMAND = writeExpr NUMBER
    COMMAND = goTo <index (ADDRESS)>
    COMMAND = repeatFor <register index (NUMBER)> <action adr (ADDRESS)>
    COMMAND = progressHue
    COMMAND = recordMacroDelay
    COMMAND = {startRecording | startRecordingBlind} [<slot identifier (MACROID)>]
    COMMAND = {recordMacro | recordMacroBlind} [<slot identifier (MACROID)>]
    COMMAND = {stopRecording | stopRecordingBlind}
    COMMAND = playMacro [<slot identifier (MACROID)>]
    COMMAND = {startMouse|stopMouse} {move DIRECTION|scroll DIRECTION|accelerate|decelerate}
    COMMAND = {setReg|addReg|subReg|mulReg} <register index (NUMBER)> <value (NUMBER)>
    COMMAND = {pressKey|holdKey|tapKey|releaseKey} SHORTCUT
    COMMAND = tapKeySeq [SHORTCUT]+
    COMMAND = set module.MODULEID.navigationMode.LAYERID NAVIGATIONMODE
    COMMAND = set module.MODULEID.baseSpeed <speed multiplier part that always applies, 0-10.0 (FLOAT)>
    COMMAND = set module.MODULEID.speed <speed multiplier part that is affected by xceleration, 0-10.0 (FLOAT)>
    COMMAND = set module.MODULEID.xceleration <exponent 0-1.0 (FLOAT)>
    COMMAND = set module.MODULEID.caretSpeedDivisor <1-100 (FLOAT)>
    COMMAND = set module.MODULEID.scrollSpeedDivisor <1-100 (FLOAT)>
    COMMAND = set module.MODULEID.axisLockSkew <0-2.0 (FLOAT)>
    COMMAND = set module.MODULEID.axisLockFirstTickSkew <0-2.0 (FLOAT)>
    COMMAND = set module.MODULEID.scrollAxisLock BOOLEAN
    COMMAND = set module.MODULEID.cursorAxisLock BOOLEAN
    COMMAND = set module.MODULEID.caretAxisLock BOOLEAN
    COMMAND = set module.MODULEID.swapAxes BOOLEAN
    COMMAND = set module.MODULEID.invertScrollDirection BOOLEAN
    COMMAND = set module.touchpad.pinchZoomDivisor <1-100 (FLOAT)>
    COMMAND = set module.touchpad.pinchZoomMode NAVIGATIONMODE
    #NOTIMPLEMENTED COMMAND = set secondaryRoles
    COMMAND = set mouseKeys.{move|scroll}.initialSpeed <px/s, -100/20 (NUMBER)>
    COMMAND = set mouseKeys.{move|scroll}.baseSpeed <px/s, -800/20 (NUMBER)>
    COMMAND = set mouseKeys.{move|scroll}.initialAcceleration <px/s, ~1700/20 (NUMBER)>
    COMMAND = set mouseKeys.{move|scroll}.deceleratedSpeed <px/s, ~200/10 (NUMBER)>
    COMMAND = set mouseKeys.{move|scroll}.acceleratedSpeed <px/s, ~1600/50 (NUMBER)>
    COMMAND = set mouseKeys.{move|scroll}.axisSkew <multiplier, 0.5-2.0 (FLOAT)>
    COMMAND = set diagonalSpeedCompensation BOOLEAN
    COMMAND = set chordingDelay <time in ms (NUMBER)>
    COMMAND = set stickyModifiers {never|smart|always}
    COMMAND = set debounceDelay <time in ms, at most 250 (NUMBER)>
    COMMAND = set doubletapTimeout <time in ms, at most 65535 (NUMBER)>
    COMMAND = set keystrokeDelay <time in ms, at most 65535 (NUMBER)>
    COMMAND = set autoRepeatDelay <time in ms, at most 65535 (NUMBER)>
    COMMAND = set autoRepeatRate <time in ms, at most 65535 (NUMBER)>
    COMMAND = set setEmergencyKey KEYID
    COMMAND = set macroEngine.scheduler {blocking|preemptive}
    COMMAND = set macroEngine.batchSize <number of commands to execute per one update cycle NUMBER>
    COMMAND = set navigationModeAction.NAVIGATIONMODECUSTOM.DIRECTION ACTION
    COMMAND = set keymapAction.LAYERID.KEYID ACTION
    COMMAND = set backlight.strategy { functional | constantRgb }
    COMMAND = set backlight.constantRgb.rgb <number 0-255 (NUMBER)> <number 0-255 (NUMBER)> <number 0-255 (NUMBER)><number 0-255 (NUMBER)>
    COMMAND = set leds.enabled BOOLEAN
    COMMAND = set leds.brightness <0-1 multiple of default (FLOAT)>
    COMMAND = set leds.fadeTimeout <minutes to fade after (NUMBER)>
    COMMAND = set modifierLayerTriggers.{shift|alt|super|control} {left|right|both}
    CONDITION = {ifShortcut | ifNotShortcut} [IFSHORTCUTFLAGS]* [KEYID]+
    CONDITION = {ifGesture | ifNotGesture} [IFSHORTCUTFLAGS]* [KEYID]+
    CONDITION = {ifPrimary | ifSecondary}
    CONDITION = {ifDoubletap | ifNotDoubletap}
    CONDITION = {ifInterrupted | ifNotInterrupted}
    CONDITION = {ifReleased | ifNotReleased}
    CONDITION = {ifKeyActive | ifNotKeyActive} KEYID
    CONDITION = {ifKeyDefined | ifNotKeyDefined} KEYID
    CONDITION = {ifKeyPendingAt | ifNotKeyPendingAt} <idx in buffer (NUMBER)> KEYID
    CONDITION = {ifPending | ifNotPending} <n (NUMBER)>
    CONDITION = {ifPendingKeyReleased | ifNotPendingKeyReleased} <queue idx (NUMBER)>
    CONDITION = {ifPlaytime | ifNotPlaytime} <timeout in ms (NUMBER)>
    CONDITION = {ifShift | ifAlt | ifCtrl | ifGui | ifAnyMod | ifNotShift | ifNotAlt | ifNotCtrl | ifNotGui | ifNotAnyMod}
    CONDITION = {ifRegEq | ifNotRegEq | ifRegGt | ifRegLt} <register index (NUMBER)> <value (NUMBER)>
    CONDITION = {ifKeymap | ifNotKeymap} KEYMAPID
    CONDITION = {ifLayer | ifNotLayer} LAYERID
    CONDITION = {ifRecording | ifNotRecording}
    CONDITION = {ifRecordingId | ifNotRecordingId} MACROID
    MODIFIER = suppressMods
    MODIFIER = postponeKeys
    MODIFIER = final
    MODIFIER = autoRepeat
    IFSHORTCUTFLAGS = noConsume | transitive | anyOrder | orGate | timeoutIn <time in ms (NUMBER)> | cancelIn <time in ms(NUMBER)>
    DIRECTION = {left|right|up|down}
    LAYERID = {fn|mouse|mod|base|fn2|fn3|fn4|fn5|alt|shift|super|control}|last|previous
    KEYMAPID = <abbrev>|last
    MACROID = last|CHAR|NUMBER
    NUMBER = [0-9]+ | -[0-9]+ | #<register idx (NUMBER)> | #key | @<relative macro action index(NUMBER)> | %<key idx in postponer queue (NUMBER)>
    BOOLEAN = 0 | 1
    FLOAT = [0-9]+{.[0-9]+} | -FLOAT
    CHAR = <any nonwhite ascii char>
    KEYID = <id of hardware key obtained by resolveNextKeyId (NUMBER)>
    LABEL = <string identifier>
    SHORTCUT = MODMASK- | MODMASK-KEY | KEY | MODMASK
    MODMASK = [MODMASK]+ | [L|R]{S|C|A|G} | {p|r|h|t} | {s|i|o}
    NAVIGATIONMODE = cursor | scroll | caret | media | zoom | zoomPc | zoomMac | none
    NAVIGATIONMODECUSTOM = caret | media | zoomPc | zoomMac
    MODULEID = trackball | touchpad | trackpoint | keycluster
    KEY = CHAR|KEYABBREV
    ADDRESS = LABEL|NUMBER
    ACTION = { macro MACROID | keystroke SHORTCUT | none }
    KEYABBREV = enter | escape | backspace | tab | space | minusAndUnderscore | equalAndPlus | openingBracketAndOpeningBrace | closingBracketAndClosingBrace
    KEYABBREV = backslashAndPipeIso | backslashAndPipe | nonUsHashmarkAndTilde | semicolonAndColon | apostropheAndQuote | graveAccentAndTilde | commaAndLessThanSign
    KEYABBREV = dotAndGreaterThanSign | slashAndQuestionMark | capsLock | printScreen | scrollLock | pause | insert | home | pageUp | delete | end | pageDown | numLock
    KEYABBREV = nonUsBackslashAndPipe | application | power | keypadEqualSign |  execute | help | menu | select | stop | again | undo | cut | copy | paste | find | mute
    KEYABBREV = volumeUp | volumeDown | lockingCapsLock | lockingNumLock | lockingScrollLock | keypadComma | keypadEqualSignAs400 | international1 | international2
    KEYABBREV = international3 | international4 | international5 | international6 | international7 | international8 | international9 | lang1 | lang2 | lang3 | lang4 | lang5
    KEYABBREV = lang6 | lang7 | lang8 | lang9 | alternateErase | sysreq | cancel | clear | prior | return | separator | out | oper | clearAndAgain | crselAndProps | exsel
    KEYABBREV = keypad00 | keypad000 | thousandsSeparator | decimalSeparator | currencyUnit | currencySubUnit | keypadOpeningParenthesis | keypadClosingParenthesis
    KEYABBREV = keypadOpeningBrace | keypadClosingBrace | keypadTab | keypadBackspace | keypadA | keypadB | keypadC | keypadD | keypadE | keypadF | keypadXor | keypadCaret
    KEYABBREV = keypadPercentage | keypadLessThanSign | keypadGreaterThanSign | keypadAmp | keypadAmpAmp | keypadPipe | keypadPipePipe | keypadColon | keypadHashmark
    KEYABBREV = keypadSpace | keypadAt | keypadExclamationSign | keypadMemoryStore | keypadMemoryRecall | keypadMemoryClear | keypadMemoryAdd | keypadMemorySubtract
    KEYABBREV = keypadMemoryMultiply | keypadMemoryDivide | keypadPlusAndMinus | keypadClear | keypadClearEntry | keypadBinary | keypadOctal | keypadDecimal
    KEYABBREV = keypadHexadecimal | keypadSlash | keypadAsterisk | keypadMinus | keypadPlus | keypadEnter | keypad1AndEnd | keypad2AndDownArrow | keypad3AndPageDown
    KEYABBREV = keypad4AndLeftArrow | keypad5 | keypad6AndRightArrow | keypad7AndHome | keypad8AndUpArrow | keypad9AndPageUp | keypad0AndInsert | keypadDotAndDelete
    KEYABBREV = leftControl | leftShift | leftAlt | leftGui | rightControl | rightShift | rightAlt | rightGui
    KEYABBREV = up | down | left | right | upArrow | downArrow | leftArrow | rightArrow
    KEYABBREV = np0 | np1 | np2 | np3 | np4 | np5 | np6 | np7 | np8 | np9
    KEYABBREV = f1 | f2 | f3 | f4 | f5 | f6 | f7 | f8 | f9 | f10 | f11 | f12 | f13 | f14 | f15 | f16 | f17 | f18 | f19 | f20 | f21 | f22 | f23 | f24
    KEYABBREV = mediaVolumeMute | mediaVolumeUp | mediaVolumeDown | mediaRecord | mediaFastForward | mediaRewind | mediaNext | mediaPrevious | mediaStop | mediaPlayPause | mediaPause
    KEYABBREV = systemPowerDown | systemSleep | systemWakeUp
    KEYABBREV = mouseBtnLeft | mouseBtnRight | mouseBtnMiddle | mouseBtn4 | mouseBtn5 | mouseBtn6 | mouseBtn7 | mouseBtn8
    MACRONAME = <Case sensitive macro identifier as named in Agent. Identifier shall not contain spaces.>
    ############
    #DEPRECATED#
    ############
    COMMAND = set doubletapDelay <time in ms, at most 65535, alias to doubletapTimeout (NUMBER)>
    COMMAND = switchLayer LAYERID
    COMMAND = switchKeymapLayer KEYMAPID LAYERID
    COMMAND = resolveNextKeyEq <queue position (NUMBER)> KEYID {<time in ms>|untilRelease} <action adr (ADDRESS)> <action adr (ADDRESS)>
    ##########
    #REMOVEWD#
    ##########
    COMMAND = setExpDriver <baseSpeed (FLOAT:0.0)> <speed (FLOAT:1.0)> <acceleration (FLOAT:0.5)> <midSpeed (FLOAT:3000)>
    COMMAND = setSplitCompositeKeystroke {0|1}
    COMMAND = setActivateOnRelease {0|1}
    MODIFIER = suppressKeys
    COMMAND = setStickyModsEnabled {0|never|smart|always|1}
    COMMAND = setCompensateDiagonalSpeed {0|1}
    COMMAND = setDebounceDelay <time in ms, at most 250 (NUMBER)>
    COMMAND = setKeystrokeDelay <time in ms, at most 65535 (NUMBER)>
    COMMAND = setReg <register index (NUMBER)> <value (NUMBER)>
    COMMAND = setEmergencyKey KEYID

### Uncategorized commands:

- `setLedTxt <time> <custom text>` will set led display to supplemented text for the given time. (Blocks for the given time.)
    - If the given time is zero, i.e. `<time> = 0`, the led text will be set indefinitely (until the display is refreshed by other text) and this command will returns immediately (non-blocking).
- `progressHue` or better `autoRepeat progressHue` will slowly adjust constantRGB value in order to rotate the per-key-RGB backlight through all hues.
- `resetTrackpoint` resets the internal trackpoint board. Can be used to recover the trackpoint from drift conditions. Drifts usually happen if you keep the cursor moving at slow constant speeds, because of the boards's internal adaptive calibration. Since the board's parameters cannot be altered, the only way around is or you to learn not to do the type of movement which triggers them.

### Triggering keyboard actions (pressing keys, clicking, etc.):

- `write <custom text>` will type rest of the string. Same as the plain text command. This is just easier to use with conditionals... If you want to interpolate register values, use (e.g.) `setStatus Register 0 contains #0; printStatus`.
- `writeExpr NUMBER` serves for writing out contents of registers or otherwise computed numbers. E.g., `writeExpr #5` or `writeExpr @-2`.
- `startMouse/stopMouse` start/stop corresponding mouse action. E.g., `startMouse move left`
- `pressKey|holdKey|tapKey|releaseKey` Presses/holds/taps/releases the provided scancode. E.g., `pressKey mouseBtnLeft`, `tapKey LC-v` (Left Control + (lowercase) v), `tapKey CS-f5` (Ctrl + Shift + F5), `LS-` (just tap left Shift).
  - press means adding the scancode into a list of "active keys" and continuing the macro. The key is released once the macro ends. I.e., if the command is not followed by any sort of delay, the key will be released again almost immediately.
  - release means removing the scancode from the list of "active keys". I.e., it negates effect of `pressKey` within the same macro. This does not affect scancodes emited by different keyboard actions.
  - tap means pressing a key (more precisely, activating the scancode) and immediately releasing it again
  - hold means pressing the key, waiting until key which activated the macro is released and then releasing the key again. I.e., `holdKey <x>` is equivalent to `pressKey <x>; delayUntilRelease; releaseKey <x>`, while `tapKey <x>` is equivalent to `pressKey <x>; releaseKey <x>`.
  - tapKeySeq can be used for executing custom sequences. Default action for each shortcut in sequence is tap. Other actions can be specified using `MODMASK`. E.g.:
    - `CS-u 1 2 3 space` - control shift U + number + space - linux shortcut for custom unicode character.
    - `pA- tab tab rA-` - tap alt tab twice to bring forward second background window.
  - `MODMASK` meaning:
    - `{S|C|A|G}` - Shift Control Alt Gui. (Windows, Super and Gui are the same thing. ) 
    - `[L|R]` - Left Right (which hand side modifier should be used)
    - `{s|i|o}` - modifiers (ctrl, alt, shift, gui) exist in three composition modes within UHK - sticky, input, output:
        - sticky modifiers are modifiers of composite shortcuts. These are applied only until next key press. In certain contexts, they will take effect even after their activation key was released (e.g., to support alt + tab on non-base layers).
        - input modifiers are queried by `ifMod` conditions, and can be suppressed by `suppressMods`.
        - output modifiers are ignored by `ifMod` conditions, and are not suppressed by `suppressMods`.
        By default:
        - modifiers of normal non-macro scancode actions are treated as `sticky` when accompanied by a scancode.
        - normal non-macro modifiers (not accompanied by a scancode) are treated as `input` by default.
        - macro modifiers are treated as `output`.
    - `{p|r|h|t}` - press release hold tap - by default corresponds to the command used to invoke the sequence, but can be overriden for any.

### Control flow, macro execution (aka "functions"):

- `goTo ADDRESS` will go to action index int. Actions are indexed from zero. See `ADDRESS`
- `repeatFor <register index> ADDRESS` - abbreviation to simplify cycles. Will decrement the supplemented register and perform `goTo` to `adr` if the value is still greater than zero. Intended usecase - place after command which is to be repeated with the register containing number of repeats and adr `@-1` (or similar).
- `break` will end playback of the current macro
- `noOp` does nothing - i.e., stops macro for exactly one update cycle and then continues.
- `yield` forces macro to yield, if blocking scheduler is used. With preemptive scheduler acts just as `noOp`.
- `exec MACRONAME` will execute different macro in current state slot. I.e., the macro will be executed in current context and will *not* return. First action of the called macro is executed within the same eventloop cycle.
- `call MACRONAME` will execute another macro in a new state slot and enters sleep mode. After the called macro finishes, the control returns to the caller macro. First action of the called macro is executed within the same eventloop cycle. The called macro has its own context (e.g., its own ifInterrupted flag, its own postponing counter and flags etc.) Beware, the state pool is small - do not use deep call trees!
- `fork MACRONAME` will execute another macro in a new state slot, without entering sleep mode.
- `stopAllMacros` interrupts all macros.

### Status buffer/Debugging tools

- `printStatus` will "type" content of status buffer (256 or 1024 chars, depends on my mood) on the keyboard. Mainly for debug purposes.
- `{setStatus | setStatusPart} <custom text>` will append <custom text> to the status buffer, if there is enough space for that. This text can then be printed by `printStatus`. This command interpolates register expressions. `setStatus` automatically appends newline, `setStatusPart` does not.
- `clearStatus` will clear the buffer.
- `statsRuntime` will output information about runtime of current macro into the status buffer. The time is measured before the printing mechanism is initiated.
- `statsLayerStack` will output information about layer stack (into the buffer).
- `statsPostponerStack` will output information about postponer queue (into the buffer).
- `statsActiveKeys` will output all active keys and their states (into the buffer).
- `statsActiveMacros` will output all active macros (into the buffer).
- `statsRegs` will output content of all registers (into the buffer).
- `diagnose` will deactivate all keys and macros and print diagnostic information into the status buffer.
- `set emergencyKey KEYID` will make the one key be ignored by postponing mechanisms. `diagnose` command on such key can be used to recover keyboard from conditions like infinite postponing loop...

### Delays:

- `delayUntil <timeout>` sleeps the macro until timeout (in ms) is reached.
- `delayUntilRelease` sleeps the macro until its activation key is released. Can be used to set action on key release.
- `delayUntilReleaseMax <timeout>` same as `delayUntilRelease`, but is also broken when timeout (in ms) is reached.

### Layer/Keymap switching:

Layer/Keymap switching mechanism allows toggling/switching of keymaps or layers. We keep layer records in a stack of limited size, which can be used for nested toggling and/or holds.

special ids:
- `previous` refers to the second stack record (i.e., `stackTop-1`)
- `last` always refers to the previously used layer/keymap (i.e., `stackTop-1` or `stackTop+1`)

terminology:
- switch means loading the target keymap and reseting layer-switching context
- toggle refers to activating a layer and remaining there (plus the activated layer is pushed onto layer stack)
- hold refers to activating a layer, waiting until the key is released and then switching back (plus the activated layer is pushed onto layer stack and then removed again)

implementation details:
- layer stack contains information about switch type (held  or toggle) and a boolean which indicates whether the record is active. Once hold ends or untoggle is issued, the corresponding record (not necessarily the top record) is marked as "inactive". Whenever some record is marked inactive, all inactive records are poped from top of the stack.
- the stack contains both layer id and keymap id. If keymap ids of previous/current records do not match, full keymap is reloaded.

Commands:
- `switchKeymap` will load the keymap by its abbreviation and reset the stack.
- `switchLayer/switchKeymapLayer` are deprecated. They simply push the layer onto stack (or pop in case of `previous`) without any further handling. Should be replaced by toggle/untoggle/hold layer commands.
- `toggleLayer` toggles the layer.
- `unToggleLayer` pops topmost non-held layer from the stack. (I.e., untoggles layer which was toggled via "toggle" or "switch" feature.)
- `toggleKeymapLayer` toggles layer from different keymap.
- `holdLayer LAYERID` mostly corresponds to the sequence `toggleLayer <layer>; delayUntilRelease; unToggleLayer`, except for more elaborate conflict resolution (releasing holds in incorrect order).
- `holdKeymapLayer KEYMAPID LAYERID` just as holdLayer, but allows referring to layer of different keymap. This reloads the entire keymap, so it may be very inefficient.
- `holdLayerMax/holdKeymapLayerMax` will timeout after <timeout> ms if no action is performed in that time.
- `ifPrimary/ifSecondary` act as an abreviation for `resolveSecondary`. They use postponing mechanism and allow distinguishing between primary and secondary roles.
- `resolveSecondary <timeout in ms> [<safety margin delay in ms>] <primary action macro action index> <secondary action macro action index>` is a special action used to resolve secondary roles on alphabetic keys. The following commands are supposed to determine behaviour of primary action and the secondary role. The command takes liberty to wait for the time specified by the first argument. If the key is held for more than the time, or if the algorithm decides that secondary role should be activated, goTo to secondary action is issued. Otherwise goTo to primary action is issued. Actions are indexed from 0. Any keys pressed during resolution are postponed until the first command after the jump is performed. See examples.

  In more detail, the resolution waits for the first key release - if the switch key is released first or within the safety margin delay after release of the postponed key, it is taken for a primary action and goes to the section of the "primary action", then the postponed key is activated; if the postponed key is released first, then the switcher branches the secondary role (e.g., activates layer hold) and then the postponed key is activated; if the time given by first argument passes, the "secondary" branch is activated as in the previous case.

  - `arg1` - total timeout of the resolution. If the timeout is exceeded and the switcher key (the key which activated the macro) is still being held, goto to secondary action is issued. Recommended value is 350ms.
  - `arg2` - safety margin delay. If the postponed key is released first, we still wait for this delay (or for timeout of the arg1 delay - whichever happens first). If the switcher key is released within this delay (starting counting at release of the key), the switcher key is still taken to have been released first. Valid value is between 0 and `arg1`, meaningful values are approximately between 0 and `arg1/2`. If only three arguments are passed, this argument defaults to `arg1`.
  - `arg3`/`arg4` - primary/secondary action macro action index. When the resolution is finished, the macro jumps to one of the two indices (I.e., this command is a conditional goTo.).

### Postponing mechanisms.

We allow postponing key activations in order to allow deciding between some scenarios depending on the next pressed key and then activating the keys pressed in "past" in the newly determined context. The postponing mechanism happens in key state preprocessing phase - i.e., works prior to activation of the key's action, including all macros. Postponing mechanism registers and postpones both key presses and releases, but does not preserve delays between them. Postponing affects even macro keystate queries, unless the macro in question is the one which initiates the postponing state (otherwise `postponeKeys delayUntilRelease` would indefinitely postpone its own release). Replay of postponed keys happens every `CYCLES_PER_ACTIVATION` update cycles, currently 2.The following commands either use this feature or allow control of the queue.

- `postponeKeys` modifier prefixed before another command keeps the firmware in postponing mode. Once no instance of postponeKeys modifer is active, the postponer will start replaying the keys. Replaying happens with normal event loop, which means that postponed keys will be replayed even during macro execution (most likely after next macro action). Some commands (thos from this section) apply this modifier implicitly. See MODIFIER section.
- `postponeNext <n>` command will apply `postponeKeys` modifier on the current command and following next n commands (macro actions).
- `ifPending/ifNotPending <n>` is true if there is at least `n` postponed keys in the queue.
- `ifPendingKeyReleased/ifNotPendingKeyReleased <queue idx>` is true if the key pending at `idx` in queue has been released. I.e., if there exists matching release event in the queue.
- `ifKeyPendingAt/ifNotKeyPendingAt <idx> <keyId>` looks into postponing queue at `idx`th waiting key nad compares it to the `keyId`.
- `consumePending <n>` will remove n records from the queue.
- `activateKeyPostponed KEYID` will add tap of KEYID at the end of queue. If `atLayer LAYERID` is specified, action will be taken from that layer rather than current one.
- `resolveSecondary` allows resolution of secondary roles depending on the next key - this allows us to accurately distinguish random press from intentional press of shortcut via secondary role. See `resolveSecondary` entry under Layer switching. Implicitly applies `postponeKeys` modifier.
- `ifPrimary/ifSecondary` act as an abreviation for `resolveSecondary`. They use postponing mechanism and allow distinguishing between primary and secondary roles.
- `ifShortcut/ifNotShortcut/ifGesture/ifNotGesture [IFSHORTCUTFLAGS]* [KEYID]*` will wait for next keypresses until sufficient number of keys has been pressed. If the next keypresses correspond to the provided arguments (hardware ids), the keypresses are consumed and the condition is performed. Consuming takes place in both `if` and `ifNot` versions if the full list is matched. E.g., `ifShortcut 090 089 final tapKey C-V; holdKey v`.
  - `Shortcut` requires continual press of keys (e.g., like Ctrl+c). By default, timeouts with release of the activation key.
  - `Gesture` allows noncontinual sequence of keys (e.g., vim's gg). By default, timeouts in 1000 ms since activation.
  - `IFSHORTCUTFLAGS`:
    - `noConsume` allows not consuming the keys. Useful if the next action is a standalone action, yet we want to branch behaviour of current action depending on it.
    - `transitive` makes termination conditions relate to that key of the queue whose result is most permissive (normally, they always refer to the activation key) - e.g., in transitive mode with 3-key shortcut, first key can be released if second key is being held. Timers count time since last performed action in this mode. Both `timeoutIn` and `cancelIn` behave according to this flag. In non-transitive mode, timers are counted since activation key press - i.e., since macro start.
    - `anyOrder` will check only presence of mentioned keyIds in postponer queue.
    - `orGate` will treat the given list of keys as *or-conditions* (rather than as *and-conditions*). Check any presence of mentioned keyIds in postponer queue for the next key press. Implies `anyOrder`.
    - `timeoutIn <time (NUMBER)>` adds a timeout timer to both `Shortcut` and `Gesture` commands. If the timer times out (i.e., the condition does not suceed or fail earlier), the command continues as if matching KEYIDs failed. Can be used to shorten life of `Shortcut` resolution.
    - `cancelIn <time (NUMBER)>` adds a timer to both commands. If this timer times out, all related keys are consumed and macro is broken. *"This action has never happened, lets not talk about it anymore."* (Note that this is an only condition which behaves same in both `if` and `ifNot` cases.)
- DEPRECATED (use `ifShortcut/ifGesture` instead) `resolveNextKeyEq <queue idx> <key id> <timeout> <adr1> <adr2>` will wait for next (n) key press(es). When the key press happens, it will compare its id with the `<key id>` argument. If the id equals, it issues goto to adr1. Otherwise, to adr2. See examples. Implicitly applies `postponeKeys` modifier.
  - `arg1 - queue idx` idx of key to compare, indexed from 0. Typically 0, if we want to resolve the key after next key then 1, etc.
  - `arg2 - key id` key id obtained by `resolveNextKeyId`. This is static identifier of the hardware key.
  - `arg3 - timeout` timeout. If not enough keys is pressed within the time, goto to `arg5` is issued. Either number in ms, or `untilRelease`.
  - `arg4 - adr1` index of macro action to go to if the `arg1`th next key's hardware identifier equals `arg2`.
  - `arg5 - adr2` index of macro action to go to otherwise.
- `resolveNextKeyId` will wait for next key press. When the next key is pressed, it will type a unique identifier identifying the pressed hardware key.
  - E.g., create a macro containing this command, and bint it to key `a`. Focus text editor. Tap `a`, tap `b`. Now, you should see `91` in your text editor, which is `b`'s `KEYID`.

### Conditions

Conditions are checked before processing the rest of the command. If the condition does not hold, the rest of the command is skipped entirelly. If the command is evaluated multiple times (i.e., if it internally consists of multiple steps, such as the delay, which is evaluated repeatedly until the desired time has passed), the condition is evaluated only in the first iteration.

- `ifDoubletap/ifNotDoubletap` is true if the macro was started at most 300ms after start of another instance of the same macro.
- `ifInterrupted/ifNotInterrupted` is true if a keystroke action or mouse action was triggered during macro runtime. Allows fake implementation of secondary roles. Also allows interruption of cycles.
- `ifReleased/ifNotReleased` is true if the key which activated current macro has been released. If the key has been physically released but the release has been postponed by another key, the conditien yields false. If the key has been physically released and the postponing mode was initiated by this macro (e.g., `postponeKeys ifReleased goTo @2`), it returns non-postponed release state (i.e., true if there's a matching release event in the postponing queue).
- `ifPending/ifNotPending <n>` is true if there is at least `n` postponed keys in the postponing queue. In context of postponing mechanism, this condition acts similar in place of ifInterrupted.
- `ifPendingKeyReleased/ifNotPendingKeyReleased <queue idx>` is true if the key pending at `idx` in queue has been released. I.e., if there exists matching release event in the queue.
- `ifKeyPendingAt/ifNotKeyPendingAt <idx> KEYID` looks into postponing queue at `idx`th waiting key nad compares it to the `keyId`. See `resolveNextKeyId`.
- `ifKeyActive/ifNotKeyActive KEYID` is true if the key is pressed at the moment. This considers *postponed* states (I.e., reads state as processed by postponer, not reading actual hardware states).
- `ifKeyDefined/ifNotKeyDefined KEYID` is true if the key in parameter has defined action on the current keymap && layer. If you wish to test keys from different layers/keymaps, you will have to toggle them manually first.
- `ifPlaytime/ifNotPlaytime <timeout in ms>` is true if at least `timeout` milliseconds passed since macro was started.
- `ifShift/ifAlt/ifCtrl/ifGui/ifAnyMod/ifNotShift/ifNotAlt/ifNotCtrl/ifNotGui/ifNotAnyMod` is true if either right or left modifier was held in the previous update cycle. This does not indicate modifiers which were triggered from macroes.
- `{ifRegEq|ifNotRegEq} <register inex> <value>` will test if the value in the register identified by first argument equals second argument.
- `{ifRegGt|ifRegLt} <register inex> <value>` will test if the value in the register identified by first argument is greater than/less than second argument.
- `{ifKeymap|ifNotKeymap|ifLayer|ifNotLayer} <value>` will test if the current Keymap/Layer are equals to the first argument (uses the same parsing rule as `switchKeymap` and `switchLayer`.
- `ifRecording/ifNotRecording` and `ifRecordingId/ifNotRecordingId MACROID` test if the runtime macro recorder is in recording state.
- `ifShortcut/ifNotShortcut [IFSHORTCUTFLAGS]* [KEYID]*` will wait for next keypresses and compare them to the argument. See postponer mechanism section.
- `ifGesture/ifNotGesture [IFSHORTCUTFLAGS]* [KEYID]*` just as `ifShortcut`, but breaks after 1000ms instead of when the key is released. See postponer mechanism section.
- `ifPrimary/ifSecondary` act as an abreviation for `resolveSecondary`. They use postponing mechanism and allow distinguishing between primary and secondary roles.

### Modifiers

Modifiers modify behaviour of the rest of the keyboard while the rest of the command is active (e.g., a delay) is active.

- `suppressMods` will supress any modifiers except those applied via macro engine. Can be used to remap shift and nonShift characters independently.
- `postponeKeys` will postpone all new key activations for as long as any instance of this modifier is active. See postponing mechanisms section.
- `final` will end macro playback after the "modified" action is properly finished. Simplifies control flow. "Implicit break."
- `autoRepeat` will continuously repeats the following command while holding the macro key, with some configurable delay. See `set autoRepeatDelay <time>` and `set autoRepeatRate <time>` for more details. This enables you to use keyrepeat feature (which is typically implemented in the OS level) with any macro action. For example, you can use something like `autoRepeat tapKey down` or `ifShift autoRepeat tapKeySeq C-right right`.

### Runtime macros:

Macro recorder targets vim-like macro functionality.

Usage (e.g.): call `recordMacro a`, do some work, end recording by another `recordMacro a`. Now you can play the actions (i.e., sequence of keyboard reports) back by calling `playMacro a`.

nly BasicKeyboard scancodes are available at the moment. These macros are recorded into RAM only. Number of macros is limited by memory (current limit is set to approximately 500 keystrokes (4kb) (maximum is ~1000 if we used all available memory)). If less than 1/4 of dedicated memory is free, oldest macro slot is freed. If currently recorded macro is longer than 1/4 of dedicated memory, recording is stopped and the macro is freed (prevents unwanted deletion of macros).

Macro slots are identified by a single character or a number or `#key` (meaning "this key").

- `recordMacroDelay` will measure time until key release (i.e., works like `delayUntilRelease`) and insert delay of that length into the currently recorded macro. This can be used to wait for window manager's reaction etc.
- `recordMacro [<macro slot id(MACROID)>]` will toggle recording (i.e., either start or stop)
- `startRecording [<macro slot id(MACROID)>]` will stop current recording (if any) and start new
- `stopRecording` will stop recording the current macro
- If the `MACROID` argument is ommited, last id is used.
- `{startRecordingBlind | stopRecordingBlind | recordMacroBlind} ...` work similarly, except that basic scancode output of keyboard is suppressed.

### Registers:
For the purpose of toggling functionality on and off, and for global constants management, we provide 32 numeric registers (namely of type int32_t).

- `setReg <register index> <value>` will set register identified by index to value.
- `ifRegEq|ifNotRegEq|ifRegGt|ifRegLt` see CONDITION section
- `{addReg|subReg|mulReg} <register index> <value>` adds value to the register
- Register values can also be used in place of all numeric arguments by prefixing register index by '#'. E.g., waiting until release or for amount of time defined by reg 1 can be achieved by `delayUntilReleaseMax #1`

### Configuration options:

- `set stickyModifiers {never|smart|always}` globally turns on or off sticky modifiers. This affects only standard scancode actions. Macro actions (both gui and command ones) are always nonsticky, unless `sticky` flag is included in `tapKey|holdKey|pressKey` commands. Default value is `smart`, which is the official behaviour - i.e., `<alt/ctrl/gui> + <tab/arrows>` are sticky.
- `set diagonalSpeedCompensation BOOLEAN` will divide diagonal mouse speed by sqrt(2) if enabled.
- `set chordingDelay 0 | <time in ms (NUMBER)>` If nonzero, keyboard will delay *all* key actions by the specified time (recommended 50ms). If another key is pressed during this time, pending key actions will be sorted according to their type:
  1) Keymap/layer switches
  2) Macros
  3) Keystrokes and mouse actions
  This allows the user to trigger chorded shortcuts in arbitrary ordrer (all at the "same" time). E.g., if `A+Ctrl` is pressed instead of `Ctrl+A`, keyboard will still send `Ctrl+A` if the two key presses follow within the specified time.
- `set debounceDelay <time in ms, at most 250>` prevents key state from changing for some time after every state change. This is needed because contacts of mechanical switches can bounce after contact and therefore change state multiple times in span of a few milliseconds. Official firmware debounce time is 50 ms for both press and release. Recommended value is 10-50, default is 50.
- `set doubletapTimeout <time in ms, at most 65535>` controls doubletap timeouts for both layer switchers and for the `ifDoubletap` condition.
- `set keystrokeDelay <time in ms, at most 65535>` allows slowing down keyboard output. This is handy for lousily written RDP clients and other software which just scans keys once a while and processes them in wrong order if multiple keys have been pressed inbetween. In more detail, this setting adds a delay whenever a basic usb report is sent. During this delay, key matrix is still scanned and keys are debounced, but instead of activating, the keys are added into a queue to be replayed later. Recommended value is 10 if you have issues with RDP missing modifier keys, 0 otherwise.
- `set autoRepeatDelay <time in ms, at most 65535>` and `set autoRepeatRate <time in ms, at most 65535>` allows you to set the initial delay (default: 500 ms) and the repeat delay (default: 50 ms) when using `autoRepeat`. When you run the command `autoRepeat <command>`, the `<command>` is first run without delay. Then, it will waits `autoRepeatDelay` amount of time before running `<command>` again. Then and thereafter, it will waits `autoRepeatRate` amount of time before repeating `<command>` again. This is consistent with typical OS keyrepeat feature.
- `set mouseKeys.{move|scroll}.{...} NUMBER` please refer to Agent for more details
  - `initialSpeed` - the speed that is active when key is pressed
  - `initialAcceleration,baseSpeed` - when mouse key is held, speed increases until it reaches baseSpeed
  - `deceleratedSpeed` - speed as affected by deceleration modifier
  - `acceleratedSpeed` - speed as affected by acceleration modifier
  - `axisSkew` - axis skew multiplies horizontal axis and divides vertical. Default value is 1.0, reasonable between 0.5-2.0 Useful for very niche usecases.
- `set module.MODULEID.{baseSpeed|speed|xceleration}` modifies speed characteristics of right side modules. 
    
    Simply speaking, `xceleration` increases sensitivity at high speeds, while decreasing sensitivity at low speeds. Furthermore, `speed` controls contribution of the acceleration formula. The `baseSpeed` can be used to offset the low-speed-sensitivity-decrease effect by making some raw input be applied directlo to the output.

    ![speed relations](resources/mouse_speeds.svg)
    
    Actual formula is is something like `speedMultiplier(normalizedSpeed) = baseSpeed + speed*(normalizedSpeed^xceleration)` where `normalizedSpeed = actualSpeed / midSpeed`. Therefore `appliedDistance(distance d, time t) = d*(baseSpeed*((d/t)/midSpeed) + d*speed*(((d/t)/midSpeed)^xceleration))`. (`d/t` is actual speed in px/s, `(d/t)/midSpeed` is normalizedSpeed which acts as base for the exponent). 
  - `baseSpeed` makes portion of the raw input contribute directly to the output. I.e., if `speed = 0`, then traveled distance is `reportedDistance*baseSpeed`
  - `speed` multiplies effect of xceleration expression. I.e., simply multiplies the reported distance when the actual speed equals `midSpeed`.
  - `xceleration` is exponent applied to the speed normalized w.r.t midSpeed. It makes cursor move relatively slower at low speeds and faster with aggresive swipes. It increases non-linearity of the curve, yet does not alone make the cursor faster and more responsive - thence "xceleration" rather than "acceleration" to avoid confusion. I.e., xceleration expression of the formula is `speed*(reportedSpeed/midSpeed)^(xceleration)`. I.e., no acceleration is xceleration = 0, reasonable (square root) acceleration is xceleration = 0.5. Highest recommended value is 1.0.
  - `midSpeed` represents "middle" speed, where the user can easily imagine behaviour of the device (currently fixed 3000 px/s) and henceforth easily set the coefficient. At this speed, acceleration formula yields `1.0`, i.e., `speedModifier = (baseSpeed + speed)`.
  
  General guidelines are:
    - If your cursor is sluggish at low speeds, you want to:
      - either lower xceleration
      - or increase baseSpeed
    - If you struggle to cover large distance with single swipe, you want to:
      - set xceleration to either `0.5` or `1.0` (or somewhere inbetween)
      - and then increase speed till you are satisfied
    - If cursor moves non-intuitively:
      - you want to either lower xceleration (`0.5` is a reasonable value)
      - or increase baseSpeed
    - If you want to make cursor more responsive overall:
      - you want to increase speed
  (Mostly) reasonable examples (`baseSpeed speed xceleration midSpeed`):
    - `0.0 1.0 0.0 3000` (no xceleration)
      - speed multiplier is always 1x at all speeds
    - `0.0 1.0 0.5 3000` (square root multiplier)
      - starts at 0x speed multiplier - allowing for very precise movement at low speed)
      - at 3000 px/s, yields cursor speed equal to actually picked up movement
      - at 12000 px/s, cursor speed is going to be twice the movement (because `sqrt(4) = 2`)
    - `0.5 0.5 1.0 3000` (linear speedup starting at 0.5)
      - starts at 0.5x speed multipier - meaning that resulting cursor speed is half the picked up movement at low speeds
      - at 3000 px/s, speed multiplier is 1x
      - at 12000 px/s, speed multiplier is 2.5x
      - (notice that linear xceleration actually means quadratic overall curve)
    - `1.0 1.0 1.0 3000`
      - same as before, but resulting cursor speed is double. I.e., 1x at 0 speed, 2x at 3000 px/s, 5x at 12000 px/s
    - `0.0 1.0 1.0 3000` (linear speedup starting at 0)
      - again very precise at low speed
      - at 3000 px/s, speed multiplier is 1x
      - at 6000 px/s, speed multiplier is 4x
      - not recommended - the curve will behave in very non-linear fashion.
- `set module.MODULEID.{caretSpeedDivisor|scrollSpeedDivisor|zoomSpeedDivisor|swapAxes|invertScrollDirection}` modifies scrolling and caret behaviour:
    - `caretSpeedDivisor` (default: 16) is used to divide input in caret mode. This means that per one tick, you have to move by 16 pixels (or whatever the unit is). (This is furthermore modified by axisLocking skew, as well as acceleration.)
    - `scrollSpeedDivisor` (default: 8) is used to divide input in scroll mode. This means that while scrolling, every 8 pixels produce one scroll tick. (This is furthermore modified by axisLocking skew, as well as acceleration.)
    - `pinchZoomDivisor` (default: 4 (?)) is used specifically for touchpad's zoom gesture, therefore its default value is nonstandard. Only valid for touchpad.
    - `swapAxes` swaps x and y coordinates of the module. Intened use is for keycluster trackball, since sideways scrolling is easier.
    - `invertScrollDirection` inverts scroll direction...

- `set module.MODULEID.{axisLockSkew|axisLockFirstTickSkew|cursorAxisLock|scrollAxisLock}` control axis locking feature:

  When you first move in navigation mode that has axis locking enabled, axis is locked to one of the axes. Axis locking behaviour is defined by two characteristis:

  - axis skew: when axis is locked, the secondary axis value is multiplied by `axisLockSkew`. This means that in order to change locked direction (with 0.5 value), you have to produce stroke that goes at least twice as fast in the non-locked direction compared to the locked one.
  - secondary axis zeroing: whenever the locked (primary) axis produces an event, the

  Behaviour of first tick (the one that initiates mechanism) can be controlled independently. The first tick (the first event produced when axis is not yet locked) skew is applied to *both* the axis. This allows following tweaks:

  - use `axisLockFirstTickSkew = 0.5` in order to require stronger "push" at the beginning of movement. Useful for the mini trackball, since it is likely to produce an unwanted move event when you try  to just click it. With `0.5` value, it will require two roll events to activate.
  - use `axisLockFirstTickSkew = 2.0` in order to make the first event more responsive. E.g., caret mode will make the fist character move even with a very gently push, while consecutive activations will need greater momentum.

  By default, axis locking is enabled in scroll and discreet modes for right hand modules, and for scroll, caret and media modes for keycluster.

  - `axisLockSkew` controls caret axis locking. Defaults to 0.5, valid/reasonable values are 0-100, centered around 1.
  - `axisLockFirstTickSkew` - same meaning as `axisLockSkew`, but controls how axis locking applies on first tick. Nonzero value means that firt tick will require a "push" before cursor starts moving. Or will require less "force" if the value is greater than 1.
  - `cursorAxisLock BOOLEAN` - turns axis locking on for cursor mode. Not recommended, but possible.
  - `scrollAxisLock BOOLEAN` - turns axis locking on for scroll mode. Default for keycluster trackball.
  - `caretAxisLock BOOLEAN` - turns axis locking on for all discrete modes.

- Remapping keys:
  - `set navigationModeAction.{caret|media}.{DIRECTION|none} ACTION` can be used to customize caret or media mode behaviour by binding directions to macros. This action is global and reversible only by powercycling.
  - `set keymapAction.LAYERID.KEYID ACTION` can be used to remap any action that lives in standard keymap. Most remappable ids can be retrieved with `resolveNextKeyId`. Keyid can also be constructed manually - see `KEYID`. Binding applies only until next keymap switch. E.g., `set keymapAction.base.64 keystroke escape` (maps `~` key to escape), or `set keymapAction.fn.193 macro TouchpadAction` (maps touchpad twofinger action to macro `TouchpadAction`).

- `macroEngine`
  - terminology:
       - action - one action as shown in the agent.
       - subAction - some actions have multiple phases (such as tapKey which consists at least of press and release, or delay). Such actions may take multiple update cycles to complete.
       - command - in case of command action, action consists of multiple commands. Command is defined as any nonempty text line. Commands are treated as actions, which means that macro action context is resetted for every command line. Every command line has its own address.
  - `scheduler` controls how are macros executed.
    - `preemptive` default old one - freely interleaves commands. It gives every macro slot an oppotunity to execute one action or subaction or command. This means that no macro can block operation of ther macros, but comes at a cost of quirkiness and nondeterminism.
      - `batchSize` limits how many commands can be executed per one macro slot per macro engine invocation.
    - `blocking` experimental scheduler. This scheduler keeps track of macro states and allows only one macro to run at a time. If macro yields (either enters waiting state or explicitly yields), another macro gets the exclusive privilege of running. As long as there are running (nonsleeping / waiting) macros, rest of the keyboard is in postponing state.
      - `batchSize` parameter controls how many commands can be executed per one macro engine invocation. If the number is exceeded, normal update cycle resumes in postponing state. This means that if a macro takes many actions, keyboard keys get queued in a queue to be executed later in correct order.
      - Macro states roughly correspond to following:
        - In progress - means that current action progresses state of the macro - i.e., does some work. As long as macro actions/subactions/commands return this state, they can be all executed within one keyboard update cycle.
        - In progress blocking - corresponds to commands that operate on usb reports - e.g., tap keys, etc.. If macro returns with this state, macro engine allows keyboard to perform one update cycle to send usb reports. This update cycle is performed in postponing mode. Macro engine is then resumed at the same action.
        - In progress waiting - corresponds to waiting states, such as `delayUntil`, `ifGesture`, `ifSecondary`. Whenever they get running privilege, they check their state and yield, allowing rest of the keyboard to run uninterrupted. These don't initiate postponing unless it is part of their function.
        - Sleeping - if one macro calls another, caller sleeps until callee finishes.
        - Backward jump - any backward jump also yields. This should prevent unwanted endless loops, as well as need for the user to manage yielding logic manually.

- backlight:
    - `backlight.strategy { functional | constantRgb }` sets backlight strategy.
    - `backlight.constantRgb.rgb NUMBER NUMBER NUMBER` allows setting custom constant colour for entire keyboard. E.g.: `set backlight.strategy constantRgb; set backlight.constantRgb.rgb 255 0 0` to make entire keyboard shine red.

- modifier layer triggers:
    - `set modifierLayerTriggers.{shift|alt|super|control} { left | right | both }` controls whether modifier layers are triggered by left or right or either of the modifiers.

### Argument parsing rules:

- `NUMBER` is parsed as a 32 bit signed integer and then assigned into the target variable. However, the target variable is often only 8 or 16 bit unsigned. If a number is prefixed with '#', it is interpretted as a register address (index). If a number is prefixed with '@', current macro index is added to the final value. `#key` returns activation key's hardware id. If prefixed with `%`, returns keyid of nth press event in the postponer queue (e.g., `%0` returns `KEYID` of first key which is postponed but not yet activated).
- `KEYMAPID` - is assumed to be 3 characters long abbreviation of a keymap.
- `MACROID` - macro slot identifier is either a number or a single ascii character (interpretted as a one-byte value). `#key` can be used so that the same macro refers to different slots when assigned to different keys.
- `register index` is an integer in the appropriate range, used as an index to the register array.
- `custom text` is an arbitrary text starting on next non-space character and ending at the end of the text action. (Yes, this should be refactored in the future.)
- `KEYID` is a numeric id obtained by `resolveNextKeyId` macro. It can also be constructed manually, as an index (starting at zero) added to an offset of `64*slotid`.  This means that starting offsets are:

```
  RightKeyboardHalf = 0
  LeftKeyboardHalf  = 64
  LeftModule        = 128
  RightModule       = 192
```

- `SHORTCUT` is an abbreviation of a key possibly accompanied by modifiers. Describes at most one scancode action. Can be prefixed by `C/S/A/G` denoting `Control/Shift/Alt/Gui`. Mods can further be prefixed by `L/R`, denoting left or right modifier. If a single ascii character is entered, it is translated into corresponding key combination (shift mask + scancode) according to standard EN-US layout. E.g., `pressKey mouseBtnLeft`, `tapKey LC-v` (Left Control + (lowercase) V (scancode)), `tapKey CS-f5` (Ctrl + Shift + F5), `tapKey v` (V), `tapKey V` (Shift + V).
- `LABEL` is and identifier marking some lines of the macro. When a string is encountered in a context of an address, UHK looks for a command beginning by `<the string>:` and returns its addres (index). If same label is present multiple times, the next one w.r.t. currently processed command is returned.
- `ADDRESS` addresses allow jumping between macro instructions. Every action or command has its own address, numbered from zero. Formally, address is either a `NUMBER` (including `#`, `@`, etc syntaxies) or a string which denotes label identifier. Every action consumes at least one address. (Except for command action, exactly one.) Every command (non-empty line of command action) consumes one address. E.g., `goTo 0` (go to beginning), `goTo @-1` (go to previous command, since `@` resolves relative adresses to absolute), `goTo @0` (active waiting), `goTo default` (go to line which begins by `default: ...`).

### Navigation modes:

UHK modules feature four navigation modes, which are mapped by layer and module. This mapping can be changed by the `set module.MODULEID.navigationMode.LAYERID NAVIGATIONMODE` command.

- **Cursor mode** - in this mode, modules control mouse movement. Default mode for all modules except keycluster's trackball.
- **Scroll mode** - in this mode, module can be used to scroll. Default mode for mod layer. This means that apart from switching layer, your mod layer switches also make your right hand modules act as very comfortable scroll wheels. Sensitivity is controlled by the `scrollSpeedDivisor` value.
- **Caret mode** - in this mode, module produces arrow key taps. This can be used to move comfortably in text editor, since in this mode, cursor is also locked to one of the two directions, preventing unwanted line changes. Sensitivity is controlled by the `caretSpeedDivisor`, `axisLockStrengthFirstTick` and `axisLockStrength`.
- **Media mode** - in this mode, up/down directions control volume (via media key scancodes), while horizontal play/pause and switch to next track. At the moment, this mode is not enabled by default on any layer. Sensitivity is shared with the caret mode.
- **Zoom mode pc / mac** - in this mode, `Ctrl +`/`Ctrl -` or `Gui +`/`Gui -` shortcuts are produced.
- **Zoom mode** - This mode serves specifically to implement touchpad's gesture. It alternates actions of zoomPc and zoomMac modes. Can be customized via `set module.touchpad.pinchZoomMode NAVIGATIONMODE`.

Caret and media modes can be customized by `set navigationModeAction` command.


