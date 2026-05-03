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

#ifndef _BITC_PASS_H_
#define _BITC_PASS_H_

#include <stdio.h>
#include <stdbool.h>
#include <bitc/compiler.h>

/* ── direction ── */
typedef enum { DIR_PACK, DIR_UNPACK } direction_t;

/* ── pass context ── */
typedef struct {
    compiler_t*    c;
    node_t*        root;
    type_registry_t* reg;
    char           (*input_names)[64];
    int            input_count;
    bool           optimize;
} pass_ctx_t;

/* Pass 0 (pre-compile): attribute resolution */
void pass0_resolve(node_t* root, char input_names[][64], int input_count);
void pass0_validate(compiler_t* cc);

/* Pass 1 (opti): constant struct folding for DIR_PACK */
bool pass1_const_fold(compiler_t* cc, node_t* block, pass_ctx_t* ctx,
                      const char* ctx_var, const char* indent, int depth,
                      void (*emit_field)(compiler_t*, node_t*, type_registry_t*, const char*, const char*, int, direction_t),
                      void (*emit_body)(compiler_t*, node_t*, type_registry_t*, const char*, int, direction_t),
                      direction_t dir);

/* Pass 2 (opti): skip/constant merge for DIR_UNPACK */
bool pass2_skip_merge(compiler_t* cc, node_t* block, pass_ctx_t* ctx,
                      const char* ctx_var, const char* indent, int depth,
                      void (*emit_field)(compiler_t*, node_t*, type_registry_t*, const char*, const char*, int, direction_t),
                      void (*emit_body)(compiler_t*, node_t*, type_registry_t*, const char*, int, direction_t),
                      direction_t dir);

#endif /* _BITC_PASS_H_ */
