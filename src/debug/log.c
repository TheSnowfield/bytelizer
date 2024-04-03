// SPDX-License-Identifier: GPL-2.0+
/*******************************************************************************
 * This file is the part of the Bytelizer library
 *
 * (C) Copyright 2024 TheSnowfield.
 *
 * Authors: TheSnowfield <17957399+TheSnowfield@users.noreply.github.com>
 ****************************************************************************/

#include "log.h"

static void __placeholder_log(const char* _) { /* do nothing */ }
bytelizer_log_callback_t __bytelizer_log_callback = &__placeholder_log;

void bytelizer_set_log_callback(bytelizer_log_callback_t cb) {
  __bytelizer_log_callback = cb;
}
