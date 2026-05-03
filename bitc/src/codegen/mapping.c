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

#include <string.h>
#include <stdio.h>
#include <bitc/codegen_mapping.h>
#include <bitc/const.h>
#include <bitc/utils.h>

const char* get_put_function_for_type(const char* type) {
    if (!type) return NULL;
    if (strcmp(type, BIT_UINT8)  == 0) return "bytelizer_put_uint8";
    if (strcmp(type, BIT_UINT16) == 0) return "bytelizer_put_uint16";
    if (strcmp(type, BIT_UINT32) == 0) return "bytelizer_put_uint32";
    if (strcmp(type, BIT_UINT64) == 0) return "bytelizer_put_uint64";
    if (strcmp(type, BIT_INT8)   == 0) return "bytelizer_put_int8";
    if (strcmp(type, BIT_INT16)  == 0) return "bytelizer_put_int16";
    if (strcmp(type, BIT_INT32)  == 0) return "bytelizer_put_int32";
    if (strcmp(type, BIT_INT64)  == 0) return "bytelizer_put_int64";
    if (strcmp(type, BIT_STRING) == 0) return "bytelizer_put_string";
    return NULL;
}

const char* get_get_function_for_type(const char* type) {
    if (!type) return NULL;
    if (strcmp(type, BIT_UINT8)  == 0) return "bytelizer_get_uint8";
    if (strcmp(type, BIT_UINT16) == 0) return "bytelizer_get_uint16";
    if (strcmp(type, BIT_UINT32) == 0) return "bytelizer_get_uint32";
    if (strcmp(type, BIT_UINT64) == 0) return "bytelizer_get_uint64";
    if (strcmp(type, BIT_INT8)   == 0) return "bytelizer_get_int8";
    if (strcmp(type, BIT_INT16)  == 0) return "bytelizer_get_int16";
    if (strcmp(type, BIT_INT32)  == 0) return "bytelizer_get_int32";
    if (strcmp(type, BIT_INT64)  == 0) return "bytelizer_get_int64";
    return NULL;
}

bool endian_overrides(node_t* f, const char** out, const char* type_map(const char*), const char* suffix) {
    for (int i = 0; i < f->attr_count; i++) {
        const char* a = f->attrs[i];
        if (!strstr(a, "with-uint")) continue;
        if (strstr(a, "with-uint32") && strstr(a, suffix)) {
            if (strcmp(f->basetype, BIT_UINT32) == 0) { *out = type_map(BIT_UINT32); return true; }
            if (strcmp(f->basetype, BIT_INT32) == 0)  { *out = type_map(BIT_INT32); return true; }
        }
        if (strstr(a, "with-uint16") && strstr(a, suffix)) {
            if (strcmp(f->basetype, BIT_UINT16) == 0) { *out = type_map(BIT_UINT16); return true; }
            if (strcmp(f->basetype, BIT_INT16) == 0)  { *out = type_map(BIT_INT16); return true; }
        }
        if (strstr(a, "with-uint64") && strstr(a, suffix)) {
            if (strcmp(f->basetype, BIT_UINT64) == 0) { *out = type_map(BIT_UINT64); return true; }
            if (strcmp(f->basetype, BIT_INT64) == 0)  { *out = type_map(BIT_INT64); return true; }
        }
    }
    return false;
}

const char* field_put_for(node_t* f) {
    const char* base = get_put_function_for_type(f->basetype);
    if (!base || f->attr_count == 0) return base;
    const char* ov = NULL;
    if (endian_overrides(f, &ov, get_put_function_for_type, "be")) {
        char tmp[64]; snprintf(tmp, sizeof(tmp), "%s_be", ov); return xstrdup(tmp);
    }
    if (endian_overrides(f, &ov, get_put_function_for_type, "le")) {
        char tmp[64]; snprintf(tmp, sizeof(tmp), "%s_le", ov); return xstrdup(tmp);
    }
    return base;
}

const char* field_get_for(node_t* f) {
    const char* base = get_get_function_for_type(f->basetype);
    if (!base || f->attr_count == 0) return base;
    const char* ov = NULL;
    if (endian_overrides(f, &ov, get_get_function_for_type, "be")) {
        char tmp[64]; snprintf(tmp, sizeof(tmp), "%s_be", ov); return xstrdup(tmp);
    }
    if (endian_overrides(f, &ov, get_get_function_for_type, "le")) {
        char tmp[64]; snprintf(tmp, sizeof(tmp), "%s_le", ov); return xstrdup(tmp);
    }
    return base;
}

const char* attr_to_prefix(const char* attr) {
    if (!attr) return NULL;
    if (strstr(attr, "with-uint8"))                         return "prefix_uint8";
    if (strstr(attr, "with-uint16") && strstr(attr, "be"))  return "prefix_uint16be";
    if (strstr(attr, "with-uint16") && strstr(attr, "le"))  return "prefix_uint16le";
    if (strstr(attr, "with-uint32") && strstr(attr, "be"))  return "prefix_uint32be";
    if (strstr(attr, "with-uint32") && strstr(attr, "le"))  return "prefix_uint32le";

    // bytelizer has no uint64 prefix definitions
    // if (strstr(attr, "with-uint64") && strstr(attr, "be"))  return "prefix_uint64be";
    // if (strstr(attr, "with-uint64") && strstr(attr, "le"))  return "prefix_uint64le";
    return NULL;
}

bool attr_is_prefix(const char* a) { return strstr(a, "with-uint") != NULL; }