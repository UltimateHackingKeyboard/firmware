#ifndef __SHELL_H__
#define __SHELL_H__

// Typedefs:

    typedef struct {
        uint8_t keyLog;
        uint8_t statLog;
        uint8_t ledsAlwaysOn;
        uint8_t sdbState;
        uint8_t chargerState;
    } shell_t;

// Variables:

    extern shell_t Shell;

// Functions:

    extern void InitShell(void);

#endif
