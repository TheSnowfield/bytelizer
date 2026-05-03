// SPDX-License-Identifier: GPL-2.0+
/*******************************************************************************
 * This file is the part of the Bytelizer library
 *
 * (C) Copyright 2026 TheSnowfield
 * (C) Copyright 2026 [LLM] DeepSeek V4 Pro
 * (C) Copyright 2026 [LLM] MiMo V2 Omni
 *
 * Authors: TheSnowfield <17957399+TheSnowfield@users.noreply.github.com>
 ****************************************************************************/

#ifndef _BITC_H_
#define _BITC_H_

#include <bitc/utils.h>

typedef struct compiler_t compiler_t;

void gen_c(compiler_t* cc, struct node_t* root, const char* source_name, const char* plugin_path);

#endif /* _BITC_H_ */
