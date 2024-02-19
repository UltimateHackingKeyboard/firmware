#ifndef __TEST_SCREEN_H__
#define __TEST_SCREEN_H__

// Includes:

#include <inttypes.h>
#include <stdbool.h>
#include "../widgets/widget.h"

// Macros:

// Typedefs:

// Variables:

extern widget_t* TestScreen;

// Functions:

void TestScreen_Init(framebuffer_t* buffer);

#endif
