// SPDX-License-Identifier: GPL-2.0+
/*******************************************************************************
 * This file is the part of the Bytelizer library
 *
 * (C) Copyright 2024 TheSnowfield.
 *
 * Authors: TheSnowfield <17957399+TheSnowfield@users.noreply.github.com>
 ****************************************************************************/

#ifndef _BYTELIZER_COMPILER_H
#define _BYTELIZER_COMPILER_H

#include <bytelizer/common.h>

/**
 * @brief static compilation assertion
*/
#define compilation_assert(c) typedef int compilation_assert_t[(c) ? 1 : -1]

/**
 * @brief inline function
 */
#if BYTELIZER_INLINE_FUNCTIONS == true
  #ifdef __GNUC__
    #define _inline __attribute__((always_inline)) inline
  #elif _MSC_VER
    #define _inline __inline
  #elif __clang__
    #define _inline inline
  #else
    #define _inline
    #warning "Unknown compiler, inline is not supported"
  #endif
#else
  #define _inline
#endif /* BYTELIZER_INLINE_FUNCTIONS */

#endif /* _BYTELIZER_COMPILER_H */
