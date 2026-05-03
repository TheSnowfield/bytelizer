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

#ifndef _BITC_UTILS_H_
#define _BITC_UTILS_H_

#include <stddef.h>
#include <stdbool.h>

struct node_t;

/* ── base utilities ── */
void str_toupper(const char* src, char* dst, int dsz);
void die(const char* fmt, ...);
char* xstrdup(const char* s);
char* read_file(const char* path, size_t* out_len);

/* ── type classification ── */
bool is_known_basetype(const char* bt);
int  bit_type_size(const char* name);
int  field_byte_size(struct node_t* f);
int  field_base_size(struct node_t* f);
bool field_is_be(struct node_t* f);

/* ── input name resolution ── */
bool is_input_ref(const char* name, char input_names[][64], int count);

/* ── attribute search (AST walk) ── */
const char* find_attr_value(struct node_t* root, const char* key);

/* ── string formatting ── */
void   fmt_attr_value(const char* val, char* out, size_t outsz);
char*  resolve_name_tmpl(struct node_t* root, const char* tmpl);

#endif
