# Extended macro engine

Extended macro engine is developed on kareltucek/firmware and occasionally merged into the stock firmware. However, just some of its features are available in stock firmware by default (mostly just `printStatus` and `set` commands). Full engine can be built using either `make-release.js --extendedMacros` or `make CUSTOM_CFLAGS=-DEXTENDED_MACROS` or enabled by `set macroEngine.extendedCommands 1` or obtained at https://github.com/kareltucek/firmware/releases/. Needless to say, extended commands are not officially supported. These commands are also not thoroughly tested, and are more likely to be removed or reshaped in the future.

The extended engine implements:
- macro commands for most features of the keyboard
- conditionals, jumps and sync mechanisms
- runtime macro recorder implemented on scancode level, for vim-like macro functionality
- many configuration options
- ability to run multiple macros at the same time

Some of the usecases which can be achieved via these commands are:
- mimicking secondary roles
- binding actions to doubletaps
- binding arbitrary shortcuts or gestures
- binding shift and non-shift scancodes independently
- configuring custom layer switching logic, including nested layer toggling
- unlimited number of layers via referencing layers of different keymaps

## Getting started

0) Get your UHK :-). https://ultimatehackingkeyboard.com/

1) Flash the firmware via Agent. You will find the newest firmware release under "code/releases" tab of github. I.e., at https://github.com/kareltucek/firmware/releases/ .

   Agent is the UHK configuration tool. You can get it at https://github.com/UltimateHackingKeyboard/agent/releases . When you start the Agent up, go to 'firmware' and 'Choose firmware file and flash it'.

2) If you are using stock firmware and want to use full power of the engine, you need to create a macro named `$onInit` with following content. Skip this step if you are using firmware from kareltucek/firmware repository. (This is a macro event, which will be automatically executed whenever the keyboard is powercycled.):

    set macroEngine.extendedCommands 1

3) Create some macro with some command action. (And bind it in your keymap.) For instance:

    holdKey leftShift
    ifDoubletap tapKey capsLock

4) Understand how this guide and the reference manual work:

    - Use Ctrl+F (or equivalent) a lot - here, and in the [reference manual](reference-manual.md).
    - Go through the sections of the reference manual - just reading the top section lines will give you some idea about available types of commands.
    - Read through examples in order to understand how the constructs can be combined.
    - Understand how to read the stated ebnf grammar. The grammar gives you precise instructions about available features and their parameters, as well as correct syntax. Note that some commands and parameters are only mentioned in the grammar! In case you don't know anything about grammars:
        - The grammar describes a valid expression via a set of rules. At the beginning, the expression equals "BODY". Every capital word of the expression is to be "rewritten" by a corresponding rule - i.e., the identifier is to be replaced by an expression which matches right side of the rule.
        - Notation: `<>` mark informal (human-understandable) explanation of what is to be entered. `|` operator indicates choice between left and right operand. It is typically enclosed in `{}`, in order to separate the group from the rest of the rule. `[]` denote optional arguments. Especially `[]+` marks "one or more" and `[]*` arbitrary number.
        - Provided value bounds are informational only - they denote values that seem to make sense. Sometimes default values are marked.
    - If you are still not sure about some feature or syntax, do not hesitate to ask.

5) If `ERR` appears up on the display, you can retrieve description by using `printStatus` over a focused text editor. Or, using above point, just search the [reference manual](reference-manual.md) for `ERR`.

6) If you encounter a bug, let me know. There are lots of features and quite few users around this codebase - if you do not report problems you find, chances are that no one else will (since most likely no one else has noticed).

## Examples

Every nonempty line is considered as one command. Empty line, or commented line too. Empty lines are skipped. Exception is empty command action, which counts for one command.

Configuration of the keyboard can be modified globally or per-keymap by using [macro events](reference-manual.md). For instance, macro named `$onInit` may contain following speed configuration:

    //enable extended commands in case you are using stock firmware.
    set macroEngine.extendedCommands 1

    //accel driver
    set module.trackball.baseSpeed 0.5
    set module.trackball.speed 1.0
    set module.trackball.xceleration 0.5

    set module.trackpoint.baseSpeed 1.0
    set module.trackpoint.speed 0.0
    set module.trackpoint.xceleration 0.0

Playing the macro containing following will result in switching to QWR keymap.

    switchKeymap QWR

Runtime macro recorder example. In this setup, shift+key will start recording (indicated by the "adaptive mode" led), another shift+key will stop recording. Hiting the key alone will then replay the macro (e.g., a simple repetitive text edit). Alternatively, virtual register `#key` can be used as an argument in order to assign every key to different slot.

    ifShift recordMacro A
    ifNotShift playMacro A

Implementation of standard double-tap-locking hold modifier in recursive version could look like: ("Recursivity" refers to ability to toggle another layer on top of the toggled layer.)

    holdLayer fn
    ifDoubletap toggleLayer fn

Once the layer is toggled, the target layer needs to contain another macro to allow return to the base layer. For instance:

    holdLayer previous
    ifDoubletap unToggleLayer

Alternative way to implement the above example would be the following. However, using `holdLayer` for "hold" mechanisms is strongly encouraged due to more elaborate release logic:

    toggleLayer fn
    delayUntilRelease
    untoggleLayer
    ifDoubletap toggleLayer fn

Creating double-shift-to-caps may look like:

    <press Shift>
    delayUntilRelease
    <releaseShift>
    ifNotDoubletap break
    <tap CapsLock>

Or (with newer releases):

    holdKey leftShift
    ifDoubletap tapKey capsLock

Or with Mac (which requires prolonged press of caps lock):

    holdKey leftShift
    ifNotDoubletap break
    pressKey capsLock
    delayUntil 400
    releaseKey capsLock

Enables and disables compensation of diagonal speed.

    ifShift set diagonalSpeedCompensation 1
    ifNotShift set diagonalSpeedCompensation 0

Smart toggle (if tapped, locks layer; if used with a key, acts as a secondary role):

    holdLayer mouse
    ifNotInterrupted toggleLayer mouse

Regular secondary role: (Activates the secondary role immediately and if no other key is pressed prior to its release, activates the primary role on release.)

    holdLayer mouse
    ifInterrupted break
    <regular action>

Regular secondary role with prevention of accidential key taps: (Activates the secondary role immediately, but activates the primary role only if the key has been pressed for at least a certain amount of time. This could be used to emulate the [Space Cadet Shift feature](https://beta.docs.qmk.fm/using-qmk/advanced-keycodes/feature_space_cadet).)

    holdKey leftShift
    ifInterrupted break
    ifPlaytime 200 break
    tapKey S-9

You can refer to layers of different keymaps via a set of `keymapLayer` commands. E.g.:

    holdKeymapLayer QWR base

Mapping shift/nonshift scancodes independently:

    ifShift suppressMods write 4
    ifNotShift write %

Secondary role (i.e., a role which becomes active if another key is pressed with this key) can be implemented in two variants: regular and postponed.

  - Regular version can be implemented for instance as `holdLayer mouse; ifNotInterrupted tapKey enter` - it always triggers secondary role, and once the key is released it either triggers primary role or not.
  - Postponed version postpones all other keypresses until it can distinguish between primary and secondary role. This is handy for alphabetic keys, because regular version would trigger on overlaps of alphabetic keys when writing regular textx. The postponed version can be used either via `ifPrimary` and `ifSecondary` conditions, or via a `resolveSecondary`.

Postponed secondary role switch - simple version using `ifPrimary`.

    ifPrimary final holdKey a
    holdLayer mouse

Postponed secondary role switch - `resolveSeccondary` is a bit more flexible and less user-friendly version of the `ifPrimary`/`ifSecondary` command. The `resolveSecondary` will listen for some time and once it decides whether the current situation fits primary or secondary action, it will issue goTo to the "second" line (line 1 since we index from 0) or the last line (line 3). Actions are indexed from 0.

    resolveSecondary 350 1 3
    write f
    break
    holdLayer mod

You can use `ifShortcut` when you want to map an action to a combination of keys. E.g, if I want z+x to produce Control+x, z+c to produce Control+c, z+v to produce Control+v, I will map following macro on the z key:

```
    ifShortcut 88 final tapKey C-x
    ifShortcut 89 final tapKey C-c
    ifShortcut 90 final tapKey C-v
    holdKey z
```

An `ifShortcut` macro needs to be placed on the first key of the shortcut, and refers to other keys by Their hardware ids obtained by `resolveNextKeyId` command (i.e., activating the command and pressing the key while having a text editor focused). The (`ifShortcut`) command will postpone other actions until sufficient number of keys is pressed. If the pressed keys correspond to the arguments, the keys are consumed and the rest of the command performed. Otherwise, postponed keypresses are either used up by the rest of the macro or replayed back. The `final` modifier breaks the command after the "modified" `tapKey` command finishes.

Similar command can be used to implement "loose gestures" - i.e., shortcuts where the second keypress can follow without continuity of press of the first key. It suffices to replace the `ifShortcut` by `ifGesture`. Vim-like gt and gT (g+shift+t) tab switching:

    ifGesture 077 final tapKey C-pageUp
    ifGesture 085 077 final tapKey C-pageDown
    tapKey g

In the above examples, `tapKey` can be (and probably should be) replaced by `holdKey`. "Hold" activates the scancode for as long as the key is pressed while "tap" activates it just for a fraction of a second. This distinction may seem unimportant, but just as long as you don't try to play some games with it.

Complex key sequences can be achieved using `tapKeySeq`. For instance, following emoji macro (uses linux  Ctrl+U notation) - tap `thisMacro + s + h` (as shrug) to get shrugging person, or `thisMacro + s + w` to get sweaty smile.

    ifGesture 80 73 final tapKeySeq CS-u 1 f 6 0 5 space
    ifGesture 80 21 final tapKeySeq CS-u 1 f 9 3 7 space

## Advanced commands:

You can simplify writing macros by using `#` and `@` characters. The first resolves a number as an index of a register. The second interprets the number as a relative action index. For instance the following macro will write out five "a"s with 50 ms delays

    //you can comment your code via two slashes.
    ifCtrl goTo default    //goto can also go to labels, absolute adresses and relative adresses
    ifShift final tapKey a //final modifier ends the macro once the command has finished
    setReg 0 50            //store number 50 into register 0
    setReg 1 5
    tapKey a
    delayUntil #0          //the #0 is expanded to content of register 0
    repeatFor 1 @-2        //decrement register 1; if it is non-zero, return by two commands to the tapKey command
    noOp                   //note the @ character - it resolves relative address to absolute (i.e., adds current adr)
    default: tapKey b      //<string>: denotes a label, which can be used as jump target

You can use `goTo @0` as active wait loop. Consider following example. If briefly tapped, produces `@@` (play last vim macro). If held, prepends any other key tap with a `@` key. E.g., `thisMacro + p + p` produces `@p@p` (play vim macro in register p, twice).

    begin: postponeKeys ifNotPending 1 ifNotReleased goTo @0
    postponeKeys ifReleased final ifNotInterrupted ifNotPlaytime 300 tapKeySeq @ @
    postponeKeys ifPending 1 tapKey @
    postponeKeys setReg 0 %0
    ifPending 1 goTo @0
    ifKeyActive #0 goTo @0
    goTo begin

Simple active wait loop example, to simulate qmk "caps words" - a feature which acts as a caps lock, but automatically turns off on space character:

On activation key:

    pressKey LS-
    setReg 5 1
    ifRegEq 5 1 goTo @0
    #at the end of macro, the shift gets released automatically

on space:

    holdKey space
    setReg 5 0

## Per-key LEDs fun:

Colour picker for constant colours. Bound on fn+r, fn+r+r turns colour to red, fn+r+v to violet, etc..

    ifGesture transitive 75 final set backlight.constantRgb.rgb 255 32 0  // r - red
    ifGesture transitive 84 final set backlight.constantRgb.rgb 192 255 0  // g - green
    ifGesture transitive 91 final set backlight.constantRgb.rgb 128 192 255 // b - blue
    ifGesture transitive 14 final set backlight.constantRgb.rgb 255 192 0 // y - yellow
    ifGesture transitive 90 final set backlight.constantRgb.rgb 192 64 255 // v - violet
    ifGesture transitive 9 final set backlight.constantRgb.rgb 255 128 0 // o - orange
    ifGesture transitive 73 final set backlight.constantRgb.rgb 192 32 0 // w - wine
    ifGesture transitive 21 final set backlight.constantRgb.rgb 128 48 0 // b - brown
    ifGesture transitive 22 final set backlight.constantRgb.rgb 255 192 32 // n - warm white, as "normal"
    ifGesture transitive 82 final set backlight.strategy functional // f to functional backlight
    ifGesture transitive 72 final set leds.enabled 0 // q - to turn off
    ifGesture transitive 10 final set leds.enabled 1 // p - to turn back on

To see all possible UHK hues (maximum saturation), hold a key with the following macro:

    autoRepeat progressHue

In order to make UHK slowly rotate through all rainbow colors all the time, you can use the following macro:

    progressHue
    delayUntil 1000
    goTo 0

In order to prevent it from running multiple times:

    # prevent the macro from running multiple times via (randomly picked) register 22
    setReg 22 1
    delayUntil 2000
    setReg 22 0
    beginLoop:
    ifRegEq 22 1 break
    # the important part begins here
    progressHue
    delayUntil 1000
    goTo beginLoop

You can also start this from `$onInit` by `fork rotateHues` (given you have the macro named `rotateHues`). Or add following lines to the colour picker:

    # put this at the beginning of the picker, to stop rotateHues when another choice is made.
    setReg 22 1
    # start the `rotateHues` macro on 'c' - as "changing"
    ifGesture transitive 89 final fork rotateHues


# Further reading

- [reference manual](reference-manual.md)

