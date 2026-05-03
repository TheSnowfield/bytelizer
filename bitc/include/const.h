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

#ifndef _BITC_CONST_H_
#define _BITC_CONST_H_

#define BIT_UINT8   "uint8"
#define BIT_UINT16  "uint16"
#define BIT_UINT32  "uint32"
#define BIT_UINT64  "uint64"
#define BIT_INT8    "int8"
#define BIT_INT16   "int16"
#define BIT_INT32   "int32"
#define BIT_INT64   "int64"
#define BIT_STRING  "string"
#define BIT_BYTES   "bytes"

typedef struct {
    const char* name;
    int         size;
} bit_type_info_t;

#define BIT_PRIMITIVE_COUNT 10
extern const bit_type_info_t bit_primitive_types[BIT_PRIMITIVE_COUNT];

int bit_type_size(const char* name);

#endif
