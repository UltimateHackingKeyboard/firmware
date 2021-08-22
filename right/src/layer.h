#ifndef __SRC_LAYER_H__
#define __SRC_LAYER_H__

// layer.c code was refactored into layer_switcher.h


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
        LayerId_Control,
        LayerId_Alt,
        LayerId_Super,
        LayerId_Last = LayerId_Super,
        LayerId_Count = LayerId_Last + 1,
    } layer_id_t;

#endif
