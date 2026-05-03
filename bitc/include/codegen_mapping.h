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

#ifndef _BITC_CODEGEN_MAPPING_H_
#define _BITC_CODEGEN_MAPPING_H_

#include <stdbool.h>
#include <bitc/ast.h>

const char* get_put_function_for_type(const char* type);
const char* get_get_function_for_type(const char* type);
bool endian_overrides(node_t* f, const char** out, const char* type_map(const char*), const char* suffix);
const char* field_put_for(node_t* f);
const char* field_get_for(node_t* f);
const char* attr_to_prefix(const char* attr);
bool attr_is_prefix(const char* a);

#endif /* _BITC_CODEGEN_MAPPING_H_ */