# Coding standards

## Literal structure

* 4 spaces are used for tabulation. No tabs are allowed.
* Unix line endings are used.
* Curlies are always explicitly written, even for single statements.
* No trailing whitespaces at the end of lines.
* Insert closing newline at the end of files.

## Naming identifiers

Functions are written with UpperCamelCase and noun follows verb.

```
DoThis();
```

Whenever a file exposes a group of functions as a consistent API the functions should be prefixed with the group name, followed by `_`, followed by the individual function names.

```
void LedDriver_WriteBuffer(uint8_t i2cAddress, uint8_t buffer[], uint8_t size);
void LedDriver_WriteRegister(uint8_t i2cAddress, uint8_t reg, uint8_t val);
void LedDriver_SetAllLedsTo(uint8_t val);
```

Variables and function parameters are written with lowerCamelCase.

```
uint8_t myVariable;
```

Type names are written with underscores, and end with `_t`. Type members are written with lowerCamelCase.

```
typedef struct {
    uint8_t acceleration;
    uint8_t maxSpeed;
    uint8_t roles[LAYER_COUNT];
} pointer_t;
```

## If

```
if (something) {
    ...
} else {
    ...
}
```

## for

```
for (uint8_t i; i<j; i++) {
    ...
}
```

## while

```
while (condition) {
    ...
}
```

## function declaration

```
void do_this()
{
    ...
}
```

## function calls

```
myFunction();
```
