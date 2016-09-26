/*
             LUFA Library
     Copyright (C) Dean Camera, 2015.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2015  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \dir
 *  \brief Common library header files.
 *
 *  This folder contains header files which are common to all parts of the LUFA library. They may be used freely in
 *  user applications.
 */

/** \file
 *  \brief Common library convenience headers, macros and functions.
 *
 *  \copydetails Group_Common
 */

/** \defgroup Group_Common Common Utility Headers - LUFA/Drivers/Common/Common.h
 *  \brief Common library convenience headers, macros and functions.
 *
 *  Common utility headers containing macros, functions, enums and types which are common to all
 *  aspects of the library.
 *
 *  @{
 */

/** \defgroup Group_GlobalInt Global Interrupt Macros
 *  \brief Convenience macros for the management of interrupts globally within the device.
 *
 *  Macros and functions to create and control global interrupts within the device.
 */

#ifndef __LUFA_COMMON_H__
#define __LUFA_COMMON_H__

    /* Public Interface - May be used in end-application: */
        /* Macros: */

            /** Convenience macro to determine the larger of two values.
             *
             *  \attention This macro should only be used with operands that do not have side effects from being evaluated
             *             multiple times.
             *
             *  \param[in] x  First value to compare
             *  \param[in] y  First value to compare
             *
             *  \return The larger of the two input parameters
             */
            #if !defined(MAX) || defined(__DOXYGEN__)
                #define MAX(x, y)               (((x) > (y)) ? (x) : (y))
            #endif

            /** Convenience macro to determine the smaller of two values.
             *
             *  \attention This macro should only be used with operands that do not have side effects from being evaluated
             *             multiple times.
             *
             *  \param[in] x  First value to compare.
             *  \param[in] y  First value to compare.
             *
             *  \return The smaller of the two input parameters
             */
            #if !defined(MIN) || defined(__DOXYGEN__)
                #define MIN(x, y)               (((x) < (y)) ? (x) : (y))
            #endif

            #if !defined(STRINGIFY) || defined(__DOXYGEN__)
                /** Converts the given input into a string, via the C Preprocessor. This macro puts literal quotation
                 *  marks around the input, converting the source into a string literal.
                 *
                 *  \param[in] x  Input to convert into a string literal.
                 *
                 *  \return String version of the input.
                 */
                #define STRINGIFY(x)            #x

                /** Converts the given input into a string after macro expansion, via the C Preprocessor. This macro puts
                 *  literal quotation marks around the expanded input, converting the source into a string literal.
                 *
                 *  \param[in] x  Input to expand and convert into a string literal.
                 *
                 *  \return String version of the expanded input.
                 */
                #define STRINGIFY_EXPANDED(x)   STRINGIFY(x)
            #endif

            #if !defined(CONCAT) || defined(__DOXYGEN__)
                /** Concatenates the given input into a single token, via the C Preprocessor.
                 *
                 *  \param[in] x  First item to concatenate.
                 *  \param[in] y  Second item to concatenate.
                 *
                 *  \return Concatenated version of the input.
                 */
                #define CONCAT(x, y)            x ## y

                /** CConcatenates the given input into a single token after macro expansion, via the C Preprocessor.
                 *
                 *  \param[in] x  First item to concatenate.
                 *  \param[in] y  Second item to concatenate.
                 *
                 *  \return Concatenated version of the expanded input.
                 */
                #define CONCAT_EXPANDED(x, y)   CONCAT(x, y)
            #endif
#endif

/** @} */

