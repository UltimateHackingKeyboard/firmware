#ifndef __SRC_LAYER_H__
#define __SRC_LAYER_H__

// layer.c code was refactored into layer_switcher.h


// Typedefs:

    typedef enum {
        LayerId_Base,
        LayerId_Mod,
        LayerId_Fn,
        LayerId_Mouse,
        LayerId_Last = LayerId_Mouse,
        LayerId_Count = LayerId_Last + 1,
    } layer_id_t;

#endif
