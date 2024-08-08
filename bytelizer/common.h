// SPDX-License-Identifier: GPL-2.0+
/*******************************************************************************
 * This file is the part of the Bytelizer library
 *
 * (C) Copyright 2024 TheSnowfield.
 *
 * Authors: TheSnowfield <17957399+TheSnowfield@users.noreply.github.com>
 ****************************************************************************/

#ifndef _BYTELIZER_CONFIG_H
#define _BYTELIZER_CONFIG_H

#define BYTELIZER_BIG_ENDIAN 1
#define BYTELIZER_LITTLE_ENDIAN 0

#ifndef BYTELIZER_REALLOC
  /**
   * @brief Bytelizer realloc size
   * The bytelizer will prioritize using the stack to store the data,
   * if the stack is not enough, it will create a buffer to store the data on the heap.
   * This macro is used to define the minimal size when the bytelizer is creating the buffer every time
  */
  #define BYTELIZER_REALLOC 1024
#endif

#ifndef BYTELIZER_INLINE_FUNCTIONS
  /**
   * @brief Force inline functions
   * Inline functions always can reduce the calling frequency of the function,
   * Improving the performance and increasing the program size.
   */
  #define BYTELIZER_INLINE_FUNCTIONS true
#endif

#ifndef BYTELIZER_ENABLE_LOG

  /**
   * @brief Enable internal logging
   * To print the log string, please set log callback
   * using `bytelizer_set_log_callback`.
   */
  #define BYTELIZER_ENABLE_LOG
#endif

#endif /* _BYTELIZER_CONFIG_H */
