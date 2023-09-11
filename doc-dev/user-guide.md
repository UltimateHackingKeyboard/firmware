# Extended macro engine

Extended macro engine originates in kareltucek/firmware and is fully merged and enabled by default in stock firmware. Some of the commands are featured officially in Agent's sidepane reference, some are not. The latter group is provided without guarantees and may be removed or reshaped in future.

The engine implements:
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

0.1) Get your UHK :-). https://ultimatehackingkeyboard.com/

0.2) Unless you specifically wish to use the extended command set, you may wish to start with officially supported macros. In that case, please see the smart macro reference included in the right-side pane of Agent. Its quality is far superior to this document, and features various interactive widgets that allow you to construct macros by just clicking and sliding GUI elements.

1) Create some macro with some command action. (And bind it in your keymap.) For instance:

```
holdKey iLS
ifDoubletap tapKey capsLock
```

![Doubletap to caps-lock macro](resources/caps_macro.png)

2) Understand how this guide and the reference manual work:

    - Use Ctrl+F (or equivalent) a lot - here, and in the [reference manual](reference-manual.md).
    - Go through the sections of the reference manual - just reading the top section lines will give you some idea about available types of commands.
    - Read through examples in order to understand how the constructs can be combined.
    - Understand how to read the stated ebnf grammar. The grammar gives you precise instructions about available features and their parameters, as well as correct syntax. Note that some commands and parameters are only mentioned in the grammar! In case you don't know anything about grammars:
        - The grammar describes a valid expression via a set of rules. At the beginning, the expression equals "BODY". Every capital word of the expression is to be "rewritten" by a corresponding rule - i.e., the identifier is to be replaced by an expression which matches right side of the rule.
        - Notation: `<>` mark informal (human-understandable) explanation of what is to be entered. `|` operator indicates choice between left and right operand. It is typically enclosed in `{}`, in order to separate the group from the rest of the rule. `[]` denote optional arguments. Especially `[]+` marks "one or more" and `[]*` arbitrary number.
        - Provided value bounds are informational only - they denote values that seem to make sense. Sometimes default values are marked.
    - If you are still not sure about some feature or syntax, do not hesitate to ask.

3) If `ERR` appears up on the display, you can retrieve description by using `printStatus` over a focused text editor. Or, using above point, just search the [reference manual](reference-manual.md) for `ERR`.

4) If you encounter a bug, let me know. There are lots of features and quite few users around this codebase - if you do not report problems you find, chances are that no one else will (since most likely no one else has noticed).

## Examples

Every nonempty line is considered as one command. Empty line, or commented line too. Empty lines are skipped. Exception is empty command action, which counts for one command.

### Configuration

Configuration of the keyboard can be modified globally or per-keymap by using [macro events](reference-manual.md). For instance, macro named `$onInit` is executed on power-cycling and on config reload. E.g., it may look like this:

```
// accel driver
set module.trackball.baseSpeed 0.5
set module.trackball.speed 1.0
set module.trackball.xceleration 0.5

//navigation modes
set module.trackball.navigationMode.base cursor
set module.trackball.navigationMode.fn caret
set module.trackball.navigationMode.mod scroll
set module.trackball.navigationMode.mouse scroll

//axis locks
set module.trackball.scrollAxisLock 1
set module.trackball.cursorAxisLock 0

//backlight defaults
set backlight.strategy constantRgb
set backlight.constantRgb.rgb 255 192 32
set leds.fadeTimeout 1
```

To change sensitivity of modules, use the following snippet in your `$onInit` macro (to always apply), in your `$onKeymapChange QWR` macro (to change sensitivity in the QWR keymap), or in a regular macro (to change sensitivity on demand).

In order to figure out right values, please see the Agent macro reference, or search the reference manual.

```
set module.trackball.baseSpeed 0.5
set module.trackball.speed 1.0
set module.trackball.xceleration 0.5
```

This enables and disables compensation of diagonal speed depending on shift state.

```
ifShift set diagonalSpeedCompensation 1
ifNotShift set diagonalSpeedCompensation 0
```

### Binding keys and basic principles


Key lifecycle consists of two important points:

- key press
- key release

Various combinations of lifecycles are abstracted into following actions:

- `pressKey` - just activate the scancode
- `releaseKey` - just deactivate it
- `tapKey` - activate the scancode and then immediately release it
- `holdKey` - activate the scancode and hold it for as long as the key is physically held, then release it
- `delayUntilRelease` - will wait until the key that activated the macro is released

So if we wanted to replace a regular 'a' key with a macro, we would use:

```
holdKey a
```

Which is similar to:

```
pressKey a
delayUntilRelease
releaseKey a
```


If we just want to quickly produce the `a` scancode/character, we would use:

```
tapKey a
```

Which is similar to:
```
pressKey a
releaseKey a
```

Complex key sequences can be achieved using `tapKeySeq`. For instance, following emoji macro (uses linux  Ctrl+U notation) will produce a shrugging person.
```
tapKeySeq CS-u 1 f 9 3 7 space
```

With these commands, modifiers are encoded using `CSAG`, with optional `LR` for left and right side. E.g., `LC-a` means `left control + a`.

Press/release/hold/tap can be specified by `phtr` part of the mask. E.g., `tapKeySeq p-a t-b t-c r-a` means tap `b c` while holding `a`.

In macros, we also may chain multiple actions, and include various conditions.

For instance, shift which also toggles caps lock on doubletap:

```
holdKey iLS
ifDoubletap tapKey capsLock
```

Or with Mac (which requires prolonged press of caps lock):

```
holdKey iLS
ifNotDoubletap break
pressKey capsLock
delayUntil 400
releaseKey capsLock
```

_sidenote: `i` in `iLS` makes the shift behave as regular native shift, which is needed to trigger `ifShift` conditions. For more info, search reference manual for "composition modes"._

Mapping shift/nonshift scancodes independently:

```
ifShift suppressMods write 4
ifNotShift write %
```

Some common conditions are:
- `ifDoubletap` - yields true if another instance of the same macro executed lately.
- `ifInterrupted` - yields true if another key was pressed during execution of the macro.
- `ifPlaytime 100` - yields true if macro ran for at least 100 ms.
- `ifShift` - yields true if regular shift key is being held. (See composition modes in the reference for more info on how to hack more complex setups)

Conditions always exist in pairs, e.g., `ifDoubletap` and `ifNotDoubletap`, `ifPlaytime` and `ifNotPlaytime` etc.

There is also a concept of modifiers, which can modify keyboard's or command's behaviour while a command is running:

- `suppressMods` - suppresses all standard modifiers, so you can rebind a shift and nonShift independent (or on the contrary dependent!) on shift state.
- `final` - ends macro after the "modified" command. Basically a syntactic sugar.
- `postponeKeys` - in order to mess with our time machine.
- `autoRepeat` - if we need to autorepeat manually from keyboard. For instance `autoRepeat progressHue`.
- `oneShot` prolongs this key's press until another action takes place. E.g., `oneShot holdLayer mod`.

### Handling layers

In case of layer handling, macro layer switching commands have slightly different semantics from the native layer switching commands, allowing for more fine-tuned control of the behavior such as nested layer holds or toggles.

Following macro will result in switching to QWR keymap.

```
switchKeymap QWR
```

Implementation of standard double-tap-locking hold modifier in nested version could look like:

```
holdLayer fn
ifDoubletap toggleLayer fn
```

Once the layer is toggled, the target layer needs to contain another macro to allow return to the base layer. This macro also allows holding previous (usually base) layer from a toggled layer.

```
holdLayer previous
ifDoubletap untoggleLayer
```

Smart toggle (if tapped, locks layer; if used with a key, acts as a simple secondary role):

```
holdLayer mouse
ifNotInterrupted toggleLayer mouse
```

You can refer to layers of different keymaps via a set of `keymapLayer` commands. Be careful though, these reload entire keymaps, so they may pose a performance penaly, and they also reset all your in-memory configurations:

```
holdKeymapLayer QWR base
```

### Time machine

The firmware implements a postponing queue which allows some commands to act as a time machine by querying _future_ keystrokes, and decide what to do depending on those. In practice, it means waiting until more input is entered, and then either consuming those keys and doing something special, or rewinding back a bit and performing all the actions as if nothing happened.

One concept which uses this feature is _shortcuts and gestures_.

You can use `ifShortcut` when you want to map an action to a combination of keys. E.g, if I want z+x to produce Control+x, z+c to produce Control+c, z+v to produce Control+v, I will map following macro on the z key:

```
ifShortcut x final tapKey C-x
ifShortcut c final tapKey C-c
ifShortcut v final tapKey C-v
holdKey z
```

An `ifShortcut` macro needs to be placed on the first key of the shortcut, and refers to other keys by Their hardware ids. These ids can be entered in numeric form obtained by `resolveNextKeyId` command (i.e., activating the command and pressing the key while having a text editor focused), or abbreviations may be used (as shown above). The `final` modifier breaks the command after the "modified" `tapKey` command finishes.

`ifGesture` can be used to implement "loose gestures" - i.e., shortcuts where the second keypress can follow without continuity of press of the first key. Vim-like gt and gT (g+shift+t) tab switching:

```
ifGesture t final tapKey C-pageUp
ifGesture leftShift t final tapKey C-pageDown
holdKey g
```

We can also use these to implement an emoji macro which allows us to write emojis using simple abbreviations - here tap `thisMacro + s + h` (as shrug) to get shrugging person, or `thisMacro + s + w` to get sweaty smile.

```
ifGesture 80 73 final tapKeySeq CS-u 1 f 6 0 5 space
ifGesture 80 21 final tapKeySeq CS-u 1 f 9 3 7 space
...
```

In order to bind some action to doubletap, you may use two strategies:

- simple `ifDoubletap` condition, which just checks whether another instance of the same macro was activae lately. Doubletapping this will result in sequence `ab`:

```
ifDoubletap final tapKey b
holdKey a
```

- time-machine `ifGesture` condition. Doubletapping this will produce a slight delay and then output just `b`:

```
ifGesture $thisKeyId final tapKey b
holdKey a
```

Another concept which may or may not use our time machine is secondary roles, namely the advanced strategy.

### Keyids vs Scancodes

Please note the difference between key ids and scancodes. Both notation use human-readable abbreviations and are therefore easy to mix up.

- _key ids_ identify specific hardware key. Human readable key id abbreviations are assigned to match the printed key labels according to the default en-US layout. If you wish to use numeric key ids, then be sure that the numbers have always at least two digits (e.g., `ifGesture 03 ...`).
- _scancode_ abbreviations describe scancodes as communicated to the PC, mapped according to en-US scancode mapping.

For instance, if you rebind your UHK to Dvorak, then the key identified by hardware id `s` will be mapped to the `o` scancode (i.e., will read `o` in Agent, or contain `holdKey o` macro command). That is, issuing `activateKeyPostponed s` will produce the same thing as `holdKey o`. (Of course, if you furthermore choose some non-standard language mapping in your OS, you may end up with yet another character produced on screen.)

### Secondary roles

Secondary role is a role which becomes active if another key is pressed with this key. It can be implemented in following variants: 

- Using `ifInterrupted` command.
- Using native secondary role driver (via `ifPrimary`/`ifSecondary` or GUI-mapping) with one of following resolution strategies:
  - simple strategy
  - advanced strategy which uses our time machine

#### Simulating secondary roles without secondary roles

Secondary role can be implemented using `ifNotInterrupted` condition  - such macro always triggers secondary role, and once the key is released it either triggers primary role or not.

For instance:

```
holdLayer mouse
ifNotInterrupted tapKey enter
```

Regular secondary role with prevention of accidential key taps: (Activates the secondary role immediately, but activates the primary role only if the key has been pressed for at least a certain amount of time. This could be used to emulate the [Space Cadet Shift feature](https://docs.qmk.fm/#/feature_space_cadet).)

```
holdKey leftShift
ifInterrupted break
ifPlaytime 200 break
tapKey S-9
```

#### Native secondary role - simple strategy

Simple strategy can be activated by `ifSecondary simpleStrategy` or `ifPrimary simpleStrategy`, or from agent GUI.

It can be explicitly set by:

```
set secondaryRole.defaultStrategy simple
```

However, at the moment of writing this, it already is the default strategy, so even without above command, it can be activated by:

```
ifSecondary final holdLayer mouse
tapKey escape
```

This strategy does the same as the `ifNotInterrupted` version, except it activates secondary role only after the other key is pressed. This strategy does not use any complicated postponing.

Pros: it is reliable.
Cons: it will activate during writing if you put it on an alphanumeric key.

#### Native secondary role - advanced strategy

Advanced strategy postpones all other keypresses until it can distinguish between primary and secondary role. This is handy for alphabetic keys, or if the secondary role is not a no-op. This strategy can be used via `ifPrimary advancedStrategy` and `ifSecondary advancedStrategy` conditions.

It can be made default by:
```
set secondaryRole.defaultStrategy advanced
```

Then it can be activated via GUI-mapped secondary role, or simply:

```
ifPrimary final holdKey a
holdLayer mouse
```

Advanced strategy has the disadvantage that its configuration depends on typing style of the user. Please see reference manual for meaning of all its configuration values. Two example configurations follow.

Release-order configuration:

```
set secondaryRole.defaultStrategy advanced
set secondaryRole.advanced.timeout 350
set secondaryRole.advanced.timeoutAction secondary
set secondaryRole.advanced.safetyMargin 50
set secondaryRole.advanced.triggerByRelease 1
set secondaryRole.advanced.doubletapToPrimary 0
```

Above configuration distinguishes roles based on release order of the keys. I.e., `press-A, press-B, release-B, releaseA` leads to secondary action; `press-A, press-B, release-A, release-B` leads to primary action. This configuration does not mind if you release keys lazily during writing.

More conventional timeout-triggered configuration:

```
set secondaryRole.defaultStrategy advanced
set secondaryRole.advanced.timeout 200
set secondaryRole.advanced.timeoutAction secondary
set secondaryRole.advanced.safetyMargin 0
set secondaryRole.advanced.triggerByRelease 0
set secondaryRole.advanced.doubletapToPrimary 1
set secondaryRole.advanced.doubletapTime 200
```

Above configuration will trigger secondary role whenever the dual-role key is pressed for more than 200ms, i.e., just a very slightly prolonged activation will trigger secondary role.

Furthermore, this configuration allows you to activate primary role by doubletap and hold. You may want this on your space key, or other primary key that is often used to produce a row of characters.

### Advanced key binding

In order to bind action to a complex shortcut, you can bind an `ifShortcut` macro onto the first key of such shortcut, as already shown above.

In order to bind action to sequence of keys, you can use `ifGesture`.

You can also use the `set keymapAction` to dynamically rebind keys in current keymap. Such bindings will last until keymap reload.

```
set keymapAction.base.15 keystroke left
set keymapAction.base.16 keystroke down
set keymapAction.base.17 keystroke right
set keymapAction.base.8 keystroke up
```

You may also use similar syntax to bind stuff into module navigation modes. E.g., you may repurpose your mini-trackball to do some vim magic (with `VimMTL` and `VimMTR` macros):

```
set navigationModeAction.media.left macro VimMTL
set navigationModeAction.media.right macro VimMTR
set navigationModeAction.media.up none
set navigationModeAction.media.down none
set module.keycluster.navigationMode.base media
```

Similarly, you may rebind your touchpad's "right button" by using following `$onKeymapChange any` macro event:

```
//this needs to go into a macro named `$onKeymapChange any`
set keymapAction.base.192 macro TouchpadLeft
set keymapAction.base.193 macro TouchpadRight
```

### Strings, variables and expressions

Write command accepts (not exactly) bash-style interpolated strings. E.g.:

```
write "Current keystrokeDelay is $keystrokeDelay. One plus one is $(1 + 1).\n"
write 'This is literal "$" character. And here you have an apostrophe '"'"'.'
```

You can save values into named variables via following syntax:

```
setVar foo 42
write "\$foo value is $foo."
```

You can use your variables, or configuration values in commands. E.g. to control led brightness, you may:

```
ifShift set leds.brightness ($leds.brightness * 1.5 + 0.01)
ifNotShift set leds.brightness ($leds.brightness / 1.5)
```

Following operators are accepted:
- `+,-,*,/,%` - addition, subtraction, multiplication, division and modulo
- `min(),max()` - minimum, maximum, e.g. `min($a, 2, 3, 4)`
- `<,<=,>,>=` - less than, less or equal, greater than, greater or equal
- `==,!=` - equals, not equals
- `!` - unary boolean negation

### Advanced commands:

Example of using advanced features:

```
// yes, this is a totally meaningless example
ifCtrl goTo default    //goto can also go to labels, absolute adresses and relative adresses
ifShift final tapKey a //final modifier ends the macro once the command has finished
setVar foo 50            //store number 50 into variable foo
setVar bar 5
tapKey a
delayUntil $foo
repeatFor bar $currentAddress-2        //decrement $bar; if it is non-zero, return by two commands to the tapKey command
noOp                   
default: tapKey b      //<string>: denotes a label, which can be used as jump target
```

You can use `goTo $currentAddress` as an active wait loop. Consider following example. If briefly tapped, it produces `@@` (play last vim macro). If held, it prepends any other key tap with a `@` key. E.g., `thisMacro + p + p + p` produces `@p@p@p` (play vim macro in register p, twice).

```
begin: postponeKeys ifNotPending 1 ifNotReleased goTo $currentAddress
postponeKeys ifReleased final ifNotInterrupted ifNotPlaytime 300 tapKeySeq @ @
postponeKeys ifPending 1 tapKey @
postponeKeys setVar savedKey $queuedKeyId.0
ifPending 1 goTo $currentAddress
ifKeyActive $savedKey goTo $currentAddress
goTo begin
```

Simple active wait loop example, to simulate qmk "caps words" - a feature which acts as a caps lock, but automatically turns off on space character:

On activation key:

```
pressKey LS-
setVar capsActive 1
if ($capsActive) goTo $currentAddress
//at the end of macro, the shift gets released automatically
```

on space:

```
holdKey space
setVar capsActive 0
```

### Macro recorder

Runtime macro recorder allows capturing and replaying sequences of scancodes. Each such macro is identified by a number or a character (or `$thisKeyId` which resolves to the `KEYID` of the current key, therefore allowing generic key-associated macros). (Runtime macroes are unrelated to the macros that can be created via Agent.)

In this setup, shift+key will start recording (indicated by the "adaptive mode" led), another shift+key will stop recording. Hiting the key alone will then replay the macro (e.g., a simple repetitive text edit). Alternatively, `$thisKeyId` can be used as an argument in order to assign every key to different slot.

```
ifShift recordMacro A
ifNotShift playMacro A
```

Above examples can be combined into more elaborate setups. Assume we bind following `perKeyMacro` on every key in your fn layer. Furthermore assume we bind following `recordKey` macro onto the `q` key (or `mod-q`). This gives us standard (although incomplete) vim behaviour. Tap `qa` (`mod-q + a`) to start recording a macro in slot "a". Tap `q` (`mod-q`) to end recording. Tap `fn-a` to replay the macro.

```
// perKeyMacro
ifShift final recordMacro $thisKeyId
if ($qActive) final recordMacro $thisKeyId
playMacro $thisKeyId
```

```
// recordKey
ifRecording final stopRecording
if $qActive final setVar qActive 0
setVar qActive 1
toggleLayer fn
ifNotRecording ifNotPlaytime 5000 if ($qActive) goTo @0
untoggleLayer
setVar qActive 0
```

### Per-key LEDs fun:

Colour picker for constant colours. Bound on fn+r, fn+r+r turns colour to red, fn+r+v to violet, etc..

```
ifGesture r final set backlight.constantRgb.rgb 255 32 0  // r - red
ifGesture g final set backlight.constantRgb.rgb 192 255 0  // g - green
ifGesture b final set backlight.constantRgb.rgb 128 192 255 // b - blue
ifGesture y final set backlight.constantRgb.rgb 255 192 0 // y - yellow
ifGesture v final set backlight.constantRgb.rgb 192 64 255 // v - violet
ifGesture o final set backlight.constantRgb.rgb 255 128 0 // o - orange
ifGesture w final set backlight.constantRgb.rgb 192 32 0 // w - wine
ifGesture b final set backlight.constantRgb.rgb 128 48 0 // b - brown
ifGesture n final set backlight.constantRgb.rgb 255 192 32 // n - warm white, as "normal"
ifGesture f final set backlight.strategy functional // f to functional backlight
ifGesture q final set leds.enabled 0 // q - to turn off
ifGesture p final set leds.enabled 1 // p - to turn back on
```

To see all possible UHK hues (maximum saturation), hold a key with the following macro:

```
autoRepeat progressHue
```

In order to make UHK slowly rotate through all rainbow colors all the time, you can use the following macro:

```
progressHue
delayUntil 1000
goTo 0
```

Above macro will not terminate, not even when ran multiple times. In order to fix this issue, we can use some register signalling:

```
// prevent the macro from running multiple times via (randomly picked) register 22
setVar hueStopper 1
delayUntil 2000
setVar hueStopper 0
beginLoop:
if $hueStopper break
progressHue
delayUntil 1000
goTo beginLoop
```

You can also start this from `$onInit` by `fork rotateHues` (given you have the macro named `rotateHues`). Or add following lines to the colour picker:

```
// put this at the beginning of the picker, to stop rotateHues when another choice is made.
setReg 22 1
// start the `rotateHues` macro on 'c' - as "changing"
ifGesture c final fork rotateHues
```

### Executing commands over USB

One way is to use the npm script that is packed with agent source code:

1. Build the agent.
2. Navigate to `agent/packages/usb`
3. Execute `./exec-macro-command.ts "write hello world!"`

Or, in linux, you can put following script into your path... and then use it as `uhkcmd "write hello world!"`:

```
#!/bin/bash
hidraw=`grep 'UHK 60' /sys/class/hidraw/hidraw*/device/uevent | LC_ALL=C sort -h | head -n 1 | grep -o 'hidraw[0-9][0-9]*'`
echo -e "\x14$*" > "/dev/$hidraw"
```


# Further reading

- [reference manual](reference-manual.md)

