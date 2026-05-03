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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <bitc/bitc.h>
#include <bitc/const.h>
#include <bitc/ast.h>
#include <bitc/compiler.h>
#include <bitc/pass.h>
#include <bitc/codegen_types.h>
#include <bitc/utils.h>

bool pass2_skip_merge(compiler_t* cc, node_t* block, pass_ctx_t* ctx,
                      const char* ctx_var, const char* indent, int depth,
                      void (*emit_field)(compiler_t*, node_t*, type_registry_t*, const char*, const char*, int, direction_t),
                      void (*emit_body)(compiler_t*, node_t*, type_registry_t*, const char*, int, direction_t),
                      direction_t dir) {
    if (dir != DIR_UNPACK) return false;
    char (*inames)[64] = ctx->input_names;
    int ic = ctx->input_count;

    int skip_sz = 0;
    for (node_t* c = block->first_child; c; c = c->next_sibling) {
        bool foldable = false;
        int fsz = 0;
        if (c->kind == NODE_FIELD) {
            if (!c->name && !c->value && !is_declared_type(ctx->reg, c->basetype)) {
                foldable = true;
                fsz = field_base_size(c);
            } else if (c->value && !is_declared_type(ctx->reg, c->basetype)) {
                bool ref_input = false;
                if (strncmp(c->value, "$ref:", 5) == 0 && is_input_ref(c->value + 5, inames, ic))
                    ref_input = true;
                if (!ref_input) { foldable = true; fsz = field_byte_size(c); }
            }
        }
        if (foldable) {
            skip_sz += fsz;
        } else {
            if (skip_sz > 0) {
                emitf(cc, "%sbytelizer_skip(%s, %d);\n", indent, ctx_var, skip_sz);
                skip_sz = 0;
            }
            if (c->kind == NODE_FIELD) emit_field(cc, c, ctx->reg, ctx_var, indent, depth, dir);
            else emit_body(cc, c, ctx->reg, ctx_var, depth + 1, dir);
        }
    }
    if (skip_sz > 0) {
        emitf(cc, "%sbytelizer_skip(%s, %d);\n", indent, ctx_var, skip_sz);
    }
    return true;
}
