#ifndef __CUSTOM_WIDGET_H__
#define __CUSTOM_WIDGET_H__

// Includes:

#include <inttypes.h>
#include <stdbool.h>
#include "widget.h"

// Macros:

// Typedefs:

// Variables:

// Functions:

widget_t CustomWidget_Build(void (*draw)(widget_t* self, framebuffer_t* buffer));

#endif
