#ifndef SRC_LAYER_H_
#define SRC_LAYER_H_

/**
 * layer.c code was refactored into layer_switcher.h
 */

// Macros:

    #define LAYER_COUNT 4

// Typedefs:

    typedef enum {
        LayerId_Base,
        LayerId_Mod,
        LayerId_Fn,
        LayerId_Mouse,
        LayerId_Last = LayerId_Mouse,
        LayerId_Count = LayerId_Last - 1,
    } layer_id_t;

#endif /* SRC_LAYER_H_ */
