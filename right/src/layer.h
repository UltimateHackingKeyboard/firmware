#ifndef __SRC_LAYER_H__
#define __SRC_LAYER_H__

#include <stdint.h>
#include <stdbool.h>
#include "attributes.h"

// Macros:

#define IS_MODIFIER_LAYER(L) (LayerId_Shift <= (L) && (L) <= LayerId_Gui)

// Typedefs:

    typedef enum {
        LayerId_Base,
        LayerId_Mod,
        LayerId_Fn,
        LayerId_Mouse,
        LayerId_Fn2,
        LayerId_Fn3,
        LayerId_Fn4,
        LayerId_Fn5,
        LayerId_Shift,
        LayerId_Ctrl,
        LayerId_Alt,
        LayerId_Gui,
        LayerId_Last = LayerId_Gui,
        LayerId_Count = LayerId_Last + 1,
    } layer_id_t;

    typedef struct {
        uint8_t modifierLayerMask;
        bool layerIsDefined : 1;
        bool exactModifierMatch : 1;
    } ATTR_PACKED layer_config_t;

// Variables:

    extern layer_config_t LayerConfig[LayerId_Count];

#endif
