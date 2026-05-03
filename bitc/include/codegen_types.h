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

#ifndef _BITC_CODEGEN_TYPES_H_
#define _BITC_CODEGEN_TYPES_H_

#include <bitc/pass.h>

void collect_types(node_t* n, type_registry_t* reg);
node_t* find_declared_type(type_registry_t* reg, const char* name);
bool is_declared_type(type_registry_t* reg, const char* name);

#endif /* _BITC_CODEGEN_TYPES_H_ */