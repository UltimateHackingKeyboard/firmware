#ifndef __LAYER_H__
#define __LAYER_H__

// Includes:

    #include "fsl_common.h"

// Macros:

    #define LAYER_COUNT 4

// Typedefs:

    typedef enum {
        LayerId_Base,
        LayerId_Mod,
        LayerId_Fn,
        LayerId_Mouse,
    } layer_id_t;

// Variables:

    extern layer_id_t PreviousHeldLayer;

// Functions:

    layer_id_t GetActiveLayer();


#endif
