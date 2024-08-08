// SPDX-License-Identifier: GPL-2.0+
/*******************************************************************************
 * This file is the part of the Bytelizer library
 *
 * (C) Copyright 2024 TheSnowfield.
 *
 * Authors: TheSnowfield <17957399+TheSnowfield@users.noreply.github.com>
 ****************************************************************************/

#ifndef _BYTELIZER_DEBUG_LOG_H
#define _BYTELIZER_DEBUG_LOG_H

#include <stdarg.h>
#include <bytelizer/common.h>

#ifndef BYTELIZER_ENABLE_LOG
  #define __bytelizer_log(__x)
#else
  extern void __bytelizer_log(const char* str, ...);
#endif /* BYTELIZER_ENABLE_LOG */

#endif /* _BYTELIZER_DEBUG_LOG_H */
