// SPDX-License-Identifier: GPL-2.0+
/*******************************************************************************
 * This file is the part of the Bytelizer library
 *
 * (C) Copyright 2024 TheSnowfield.
 *
 * Authors: TheSnowfield <17957399+TheSnowfield@users.noreply.github.com>
 ****************************************************************************/

#ifndef _BYTELIZER_ERROR_H
#define _BYTELIZER_ERROR_H

typedef enum {
  bytelizer_ret_ok = 0,
  bytelizer_ret_out_of_memory,
  bytelizer_ret_invalid_parameter,

} bytelizer_ret_t;

#define bytelizer_ok(__x, __y) (((__x) = (__y)) == bytelizer_ret_ok)

#endif /* _BYTELIZER_ERROR_H */
