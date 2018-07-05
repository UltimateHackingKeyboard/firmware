#ifndef __RIGHT_KEY_MATRIX_H__
#define __RIGHT_KEY_MATRIX_H__

// Includes:

    #include "key_matrix.h"

// Macros:

    #define RIGHT_KEY_MATRIX_COLS_NUM 7
    #define RIGHT_KEY_MATRIX_ROWS_NUM 5
    #define RIGHT_KEY_MATRIX_KEY_COUNT (RIGHT_KEY_MATRIX_COLS_NUM * RIGHT_KEY_MATRIX_ROWS_NUM)

// Variables:

    extern key_matrix_t RightKeyMatrix;
    extern uint32_t MatrixScanCounter;

#endif
