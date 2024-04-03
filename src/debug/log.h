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

#include <bytelizer/common.h>

typedef void (* bytelizer_log_callback_t)(const char* str);

/**
 * @brief set log callback
 *
 * @param cb the callback
 */
void bytelizer_set_log_callback(bytelizer_log_callback_t cb);

#endif /* _BYTELIZER_DEBUG_LOG_H */
