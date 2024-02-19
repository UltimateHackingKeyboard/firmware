#ifndef __SPLITTER_WIDGET_H__
#define __SPLITTER_WIDGET_H__

// Includes:

#include <inttypes.h>
#include <stdbool.h>
#include "widget.h"

// Macros:

// Typedefs:

// Variables:

// Functions:

widget_t SplitterWidget_BuildVertical(widget_t* child1, widget_t* child2, uint8_t splitAt, bool splitLine);
widget_t SplitterWidget_BuildHorizontal(widget_t* child1, widget_t* child2, uint8_t splitAt, bool splitLine);

#endif
