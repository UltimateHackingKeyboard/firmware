# Signing the CLA

Before contributing to this project, you must sign [the CLA](/cla/cla-1.0.0.md).

To sign the CLA, add your GitHub username to the end of the CLA. Make sure that the usernames remain alphabetically sorted.

Then create a pull request with the title:

> Sign CLA

and with the body:

> I have read the Agreement, and fully agree to it by signing it with my GitHub username.

# Coding standards

## Literal structure

* 4 spaces are used for tabulation. No tabs are allowed.
* Unix line endings are used.
* Curlies are always explicitly written, even for single statements.
* No trailing whitespaces at the end of lines.
* Insert closing newline at the end of files.

## Naming identifiers

Extern functions and variables are written with UpperCamelCase, non-extern functions and variables are written with lowerCamelCase.

Function names are composed of a verb followed by a noun.

Non-extern functions must be declared as static.

```
DoThisExtern(void);
static doThisNonExtern(void);
uint8 ExternVariable;
uint8 nonExternVariable;
```

Whenever a file exposes a group of functions as a consistent API, their function names should be prefixed with the group name, followed by `_`, followed by the individual function names.

```
void LedDriver_WriteBuffer(uint8_t i2cAddress, uint8_t buffer[], uint8_t size);
void LedDriver_WriteRegister(uint8_t i2cAddress, uint8_t reg, uint8_t val);
void LedDriver_SetAllLedsTo(uint8_t val);
```

Function scoped variables and function parameters are written with lowerCamelCase.

```
void MyFunction(uint8_t myArg1, uint8_t myArg2)
{
    uint8_t myVariable;
    ....
}
```

Type names are written with underscores, and end with `_t`. Type members are written with lowerCamelCase.

```
typedef struct {
    uint8_t acceleration;
    uint8_t maxSpeed;
    uint8_t roles[LAYER_COUNT];
} pointer_t;
```

## Control structures

```
if (something) {
    ...
} else {
    ...
}

switch (currentMacroAction.key.type) {
    case KeystrokeType_Basic:
        addBasicScancode(currentMacroAction.key.scancode);
        break;
    case KeystrokeType_Media:
        addMediaScancode(currentMacroAction.key.scancode);
        break;
    case KeystrokeType_System:
        addSystemScancode(currentMacroAction.key.scancode);
        break;
}

for (uint8_t i = 0; i < j; i++) {
    ...
}

while (condition) {
    ...
}
```

## Function declaration

```
void doThis(void)
{
    ...
}
```

## Function calls

```
myFunction();
```

## Header file structure

Header files are composed of sections. The order of sections is fixed. Every header file is guarded by a file-wide `#ifndef`.

```
#ifndef __LED_DRIVER_H__
#define __LED_DRIVER_H__

// Includes:

    #include "fsl_gpio.h"
    ...

// Macros:

    #define LED_DRIVER_SDB_PORT PORTA
    ...

// Typedefs:

    typedef enum {
        KeystrokeType_Basic,
        KeystrokeType_Media,
        KeystrokeType_System,
    } keystroke_type_t;

// Variables:

    extern led_driver_state_t LedDriverState;
    ...

// Functions:

    void LedDriver_WriteBuffer(uint8_t i2cAddress, uint8_t buffer[], uint8_t size);
    ...

#endif
```

## Semantics

The build process must not yield any warnings, and the build must pass [on Travis](https://travis-ci.org/UltimateHackingKeyboard/firmware).

## Clang format

There is a `.clang-format` present, which allows you to format your `.c` files simply by `clang-format -i <your source file>`. (It does **not** ensure the required header file structure though!) 
