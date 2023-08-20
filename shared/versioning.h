#ifndef __VERSIONING_H__
#define __VERSIONING_H__

// Includes:

    #include "stdint.h"

// Macros:

#define MD5_CHECKSUM_LENGTH 33

#define VERSION_AT_LEAST(v, MAJ, MIN, PATCH) \
    (\
        ((v).major > (MAJ)) \
        || ((v).major == (MAJ) && (v).minor > (MIN))\
        || ((v).major == (MAJ) && (v).minor == (MIN) && (v).patch >= (PATCH))\
    )

// Typedefs:

    typedef struct {
        uint16_t major;
        uint16_t minor;
        uint16_t patch;
    } version_t;

#endif
