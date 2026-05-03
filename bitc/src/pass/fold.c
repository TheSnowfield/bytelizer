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

#define MAX_ANCHORS 256

typedef struct {
    int      offset;
    node_t*  field;
} anchor_t;

typedef struct {
    anchor_t items[MAX_ANCHORS];
    int      count;
} anchor_state_t;

static bool field_is_const(node_t* f, type_registry_t* reg, bool optimize,
                           char input_names[][64], int input_count) {
    if (!f || f->kind != NODE_FIELD) return false;
    if (f->value) {
        if (strncmp(f->value, "$ref:", 5) == 0) {
            const char* name = f->value + 5;
            if (optimize && is_input_ref(name, input_names, input_count))
                return true;
            if (!is_input_ref(name, input_names, input_count))
                return true;
            return false;
        }
        return true;
    }
    node_t* decl = find_declared_type(reg, f->basetype);
    if (!decl || !decl->first_child) return false;
    for (node_t* tf = decl->first_child; tf; tf = tf->next_sibling) {
        if (tf->kind != NODE_FIELD) continue;
        node_t* ov = NULL;
        for (node_t* of = f->first_child; of; of = of->next_sibling) {
            const char* ofname = of->name ? of->name : of->basetype;
            if (tf->name && ofname && strcmp(tf->name, ofname) == 0) { ov = of; break; }
        }
        if (ov && !ov->name && ov->basetype) {
            ov->name = ov->basetype;
            ov->basetype = xstrdup(tf->basetype);
        }
        node_t* g = ov ? ov : tf;
        if (g->value) continue;
        if (g->first_child && (g->kind == NODE_BLOCK || strcmp(tf->basetype, BIT_BYTES) == 0)) {
            bool all_c = true;
            for (node_t* fc = g->first_child; fc; fc = fc->next_sibling)
                if (!field_is_const(fc, reg, optimize, input_names, input_count)) { all_c = false; break; }
            if (all_c) continue;
        }
        if (is_declared_type(reg, g->basetype) && field_is_const(g, reg, optimize, input_names, input_count))
            continue;
        return false;
    }
    return true;
}

static bool block_is_const(node_t* b, type_registry_t* reg, bool optimize,
                           char input_names[][64], int input_count) {
    if (!b || b->kind != NODE_BLOCK || b->param) return false;
    for (node_t* c = b->first_child; c; c = c->next_sibling) {
        if (c->kind == NODE_BLOCK) {
            if (!block_is_const(c, reg, optimize, input_names, input_count)) return false;
            continue;
        }
        if (!field_is_const(c, reg, optimize, input_names, input_count)) return false;
    }
    return true;
}

static int field_const_size(node_t* f, type_registry_t* reg, bool optimize,
                            char input_names[][64], int input_count) {
    if (!f) return 0;
    if (f->value && !is_declared_type(reg, f->basetype)) return field_byte_size(f);
    if (f->kind == NODE_BLOCK) {
        int sz = 0;
        for (node_t* c = f->first_child; c; c = c->next_sibling)
            sz += field_const_size(c, reg, optimize, input_names, input_count);
        return sz;
    }
    node_t* decl = find_declared_type(reg, f->basetype);
    if (!decl || !decl->first_child) return 0;
    int sz = 0;
    for (node_t* tf = decl->first_child; tf; tf = tf->next_sibling) {
        if (tf->kind != NODE_FIELD) continue;
        node_t* ov = NULL;
        for (node_t* of = f->first_child; of; of = of->next_sibling) {
            const char* ofname = of->name ? of->name : of->basetype;
            if (tf->name && ofname && strcmp(tf->name, ofname) == 0) { ov = of; break; }
        }
        if (ov && !ov->name && ov->basetype) {
            ov->name = ov->basetype;
            ov->basetype = xstrdup(tf->basetype);
        }
        node_t* g = ov ? ov : tf;
        if (g->value) { sz += field_byte_size(g); continue; }
        if (tf->basetype && strcmp(tf->basetype, BIT_BYTES) == 0) {
            int pfx = 0;
            for (int ai = 0; ai < tf->attr_count; ai++) {
                if (strstr(tf->attrs[ai], "with-uint8"))  pfx = 1;
                else if (strstr(tf->attrs[ai], "with-uint16")) pfx = 2;
                else if (strstr(tf->attrs[ai], "with-uint32")) pfx = 4;
                else if (strstr(tf->attrs[ai], "with-uint64")) pfx = 8;
            }
            if (ov) for (int ai = 0; ai < ov->attr_count; ai++) {
                if (strstr(ov->attrs[ai], "with-uint8"))  pfx = 1;
                else if (strstr(ov->attrs[ai], "with-uint16")) pfx = 2;
                else if (strstr(ov->attrs[ai], "with-uint32")) pfx = 4;
                else if (strstr(ov->attrs[ai], "with-uint64")) pfx = 8;
            }
            sz += pfx;
            if (g->first_child) for (node_t* fc = g->first_child; fc; fc = fc->next_sibling)
                sz += field_const_size(fc, reg, optimize, input_names, input_count);
            continue;
        }
        if (g->first_child) {
            for (node_t* fc = g->first_child; fc; fc = fc->next_sibling)
                sz += field_const_size(fc, reg, optimize, input_names, input_count);
            continue;
        }
        if (is_declared_type(reg, g->basetype)) {
            sz += field_const_size(g, reg, optimize, input_names, input_count);
            continue;
        }
    }
    int outer_pfx = 0;
    for (int ai = 0; ai < f->attr_count; ai++) {
        if (strstr(f->attrs[ai], "with-uint8"))       outer_pfx = 1;
        else if (strstr(f->attrs[ai], "with-uint16"))  outer_pfx = 2;
        else if (strstr(f->attrs[ai], "with-uint32"))  outer_pfx = 4;
        else if (strstr(f->attrs[ai], "with-uint64"))  outer_pfx = 8;
    }
    return sz + outer_pfx;
}

static void write_hex(compiler_t* cc, const char* ind, uint8_t v, int* bi, int* pl) {
    if (*bi == 0) emitf(cc, "%s", ind);
    else if (*pl == 0) emitf(cc, ",\n%s", ind);
    else emitf(cc, ", ");
    emitf(cc, "0x%02X", v);
    (*pl)++; if (*pl >= 8) *pl = 0;
    (*bi)++;
}

static const char* anchor_put_name(node_t* f) {
    const char* b = f->basetype; if (!b) return NULL;
    bool be = field_is_be(f);
    static char buf[64];
    if (strcmp(b, BIT_UINT8)==0)  snprintf(buf,sizeof(buf),"bytelizer_put_uint8%s",be?"_be":"");
    else if (strcmp(b, BIT_UINT16)==0) snprintf(buf,sizeof(buf),"bytelizer_put_uint16%s",be?"_be":"");
    else if (strcmp(b, BIT_UINT32)==0) snprintf(buf,sizeof(buf),"bytelizer_put_uint32%s",be?"_be":"");
    else if (strcmp(b, BIT_UINT64)==0) snprintf(buf,sizeof(buf),"bytelizer_put_uint64%s",be?"_be":"");
    else if (strcmp(b, BIT_INT8)==0)   snprintf(buf,sizeof(buf),"bytelizer_put_int8%s",be?"_be":"");
    else if (strcmp(b, BIT_INT16)==0)  snprintf(buf,sizeof(buf),"bytelizer_put_int16%s",be?"_be":"");
    else if (strcmp(b, BIT_INT32)==0)  snprintf(buf,sizeof(buf),"bytelizer_put_int32%s",be?"_be":"");
    else if (strcmp(b, BIT_INT64)==0)  snprintf(buf,sizeof(buf),"bytelizer_put_int64%s",be?"_be":"");
    else return NULL;
    return buf;
}

static void emit_const_field(compiler_t* cc, node_t* f, const char* ind, int* bi, int* pl,
                             bool optimize, char input_names[][64], int input_count,
                             node_t* root, anchor_state_t* anchors) {
    const char* val = f->value;
    if (strncmp(val, "$ref:", 5) == 0) {
        const char* name = val + 5;
        if (optimize && is_input_ref(name, input_names, input_count)) {
            int base = field_base_size(f);
            if (base <= 0) return;
            if (anchors->count < MAX_ANCHORS) {
                anchors->items[anchors->count].offset = *bi;
                anchors->items[anchors->count].field = f;
                anchors->count++;
            }
            static const uint8_t poison[8] = {0xDE,0xAD,0xBE,0xEF,0xDE,0xAD,0xBE,0xEF};
            if (field_is_be(f)) {
                for (int i = base-1; i >= 0; i--) write_hex(cc, ind, poison[i % 4], bi, pl);
            } else {
                for (int i = 0; i < base; i++) write_hex(cc, ind, poison[i % 4], bi, pl);
            }
            return;
        }
        val = find_attr_value(root, name);
        if (!val) return;
        if (strcmp(f->basetype, BIT_STRING) == 0) {
            for (const char* s = val; *s; s++) write_hex(cc, ind, (uint8_t)*s, bi, pl);
            return;
        }
    }
    int base = field_base_size(f); if (base <= 0 || base > 8) return;
    bool be = field_is_be(f);
    char* vbuf = xstrdup(val);
    for (char* tok = strtok(vbuf, " "); tok; tok = strtok(NULL, " ")) {
        uint64_t v = strtoull(tok, NULL, 0);
        for (int i = 0; i < base; i++) {
            int sh = be ? (base - 1 - i) * 8 : i * 8;
            write_hex(cc, ind, (uint8_t)(v >> sh), bi, pl);
        }
    }
    free(vbuf);
}

static void emit_const_block(compiler_t* cc, node_t* b, type_registry_t* reg,
                             const char* ind, int* bi, int* pl, bool opt,
                             char inames[][64], int ic, node_t* root, anchor_state_t* anchors) {
    for (node_t* c = b->first_child; c; c = c->next_sibling) {
        if (c->kind == NODE_BLOCK)
            emit_const_block(cc, c, reg, ind, bi, pl, opt, inames, ic, root, anchors);
        else
            emit_const_field(cc, c, ind, bi, pl, opt, inames, ic, root, anchors);
    }
}

static void emit_const_declared(compiler_t* cc, node_t* f, type_registry_t* reg,
                                const char* ind, int* bi, int* pl, bool opt,
                                char inames[][64], int ic, node_t* root, anchor_state_t* anchors) {
    node_t* decl = find_declared_type(reg, f->basetype);
    if (!decl) return;
    int outer_pfx = 0; bool outer_be = false;
    for (int ai = 0; ai < f->attr_count; ai++) {
        if (strstr(f->attrs[ai], "with-uint8"))       outer_pfx = 1;
        else if (strstr(f->attrs[ai], "with-uint16"))  outer_pfx = 2;
        else if (strstr(f->attrs[ai], "with-uint32"))  outer_pfx = 4;
        else if (strstr(f->attrs[ai], "with-uint64"))  outer_pfx = 8;
    }
    for (int ai = 0; ai < f->attr_count; ai++)
        if (strstr(f->attrs[ai], "with-uint") && strstr(f->attrs[ai], "be")) outer_be = true;
    if (outer_pfx > 0) {
        uint64_t body64 = (uint64_t)(field_const_size(f, reg, opt, inames, ic) - outer_pfx);
        for (int i = 0; i < outer_pfx; i++) {
            int sh = outer_be ? (outer_pfx - 1 - i) * 8 : i * 8;
            write_hex(cc, ind, (uint8_t)(body64 >> sh), bi, pl);
        }
    }
    int matched[64] = {0};
    for (node_t* tf = decl->first_child; tf; tf = tf->next_sibling) {
        if (tf->kind != NODE_FIELD) continue;
        node_t* ov = NULL; int ci = 0;
        for (node_t* of = f->first_child; of; of = of->next_sibling, ci++) {
            const char* ofname = of->name ? of->name : of->basetype;
            if (tf->name && ofname && strcmp(tf->name, ofname) == 0) { ov = of; matched[ci] = 1; break; }
        }
        if (ov && !ov->name && ov->basetype) {
            ov->name = ov->basetype;
            ov->basetype = xstrdup(tf->basetype);
        }
        node_t* g = ov ? ov : tf;
        if (!ov && tf->is_default)
            fprintf(stderr, "bitc: warning: line %d: field '%s' in '%s' of type '%s' uses unset default 0\n",
                    f->line, tf->name, f->name, f->basetype);
        if (g->value && field_byte_size(g) > 0) {
            emit_const_field(cc, g, ind, bi, pl, opt, inames, ic, root, anchors);
        } else if (tf->basetype && strcmp(tf->basetype, BIT_BYTES) == 0 && g->first_child) {
            int pfx = 0;
            for (int ai = 0; ai < tf->attr_count; ai++) {
                if (strstr(tf->attrs[ai], "with-uint8"))  pfx = 1;
                else if (strstr(tf->attrs[ai], "with-uint16")) pfx = 2;
                else if (strstr(tf->attrs[ai], "with-uint32")) pfx = 4;
                else if (strstr(tf->attrs[ai], "with-uint64")) pfx = 8;
            }
            if (ov) for (int ai = 0; ai < ov->attr_count; ai++) {
                if (strstr(ov->attrs[ai], "with-uint8"))  pfx = 1;
                else if (strstr(ov->attrs[ai], "with-uint16")) pfx = 2;
                else if (strstr(ov->attrs[ai], "with-uint32")) pfx = 4;
                else if (strstr(ov->attrs[ai], "with-uint64")) pfx = 8;
            }
            int body = 0;
            for (node_t* fc = g->first_child; fc; fc = fc->next_sibling)
                body += field_const_size(fc, reg, opt, inames, ic);
            for (int i = 0; i < pfx; i++) {
                int sh = (pfx - 1 - i) * 8;
                write_hex(cc, ind, (uint8_t)(body >> sh), bi, pl);
            }
            for (node_t* fc = g->first_child; fc; fc = fc->next_sibling) {
                if (fc->kind == NODE_BLOCK)
                    emit_const_block(cc, fc, reg, ind, bi, pl, opt, inames, ic, root, anchors);
                else if (is_declared_type(reg, fc->basetype))
                    emit_const_declared(cc, fc, reg, ind, bi, pl, opt, inames, ic, root, anchors);
                else
                    emit_const_field(cc, fc, ind, bi, pl, opt, inames, ic, root, anchors);
            }
        } else if (g->first_child && is_declared_type(reg, g->basetype)) {
            emit_const_declared(cc, g, reg, ind, bi, pl, opt, inames, ic, root, anchors);
        } else if (is_declared_type(reg, g->basetype)) {
            emit_const_declared(cc, g, reg, ind, bi, pl, opt, inames, ic, root, anchors);
        }
    }
    int ci = 0;
    for (node_t* of = f->first_child; of; of = of->next_sibling, ci++)
        if (!matched[ci] && of->kind == NODE_FIELD && (of->name || of->basetype))
            fprintf(stderr, "bitc: warning: line %d: field '%s' in '%s' has no matching field in type '%s'\n",
                    of->line, of->name ? of->name : "(anonymous)", f->name, f->basetype);
}

static void emit_anchor_patches(compiler_t* cc, const char* ctx_var, const char* indent,
                                int sz, anchor_state_t* anchors) {
    if (anchors->count == 0) return;
    emitf(cc, "%s{\n", indent);
    emitf(cc, "%s  int _apos = bytelizer_tell(%s);\n", indent, ctx_var);
    for (int ai = 0; ai < anchors->count; ai++) {
        const char* aname = anchors->items[ai].field->value + 5;
        const char* aput = anchor_put_name(anchors->items[ai].field);
        if (aput)
            emitf(cc, "%s  bytelizer_seek(%s, _apos + %d);\n%s  %s(%s, %s);\n",
                    indent, ctx_var, anchors->items[ai].offset, indent, aput, ctx_var, aname);
    }
    emitf(cc, "%s  bytelizer_seek(%s, _apos + %d);\n", indent, ctx_var, sz);
    emitf(cc, "%s}\n", indent);
}

bool pass1_const_fold(compiler_t* cc, node_t* block, pass_ctx_t* ctx,
                      const char* ctx_var, const char* indent, int depth,
                      void (*emit_field)(compiler_t*, node_t*, type_registry_t*, const char*, const char*, int, direction_t),
                      void (*emit_body)(compiler_t*, node_t*, type_registry_t*, const char*, int, direction_t),
                      direction_t dir) {

    if (dir != DIR_PACK) return false;
    bool opt = ctx->optimize;
    char (*inames)[64] = ctx->input_names;
    int ic = ctx->input_count;

    bool in_block = false;
    int uid = 0, bi = 0, sz = 0, pl = 0;
    anchor_state_t anchors;
    anchors.count = 0;

    for (node_t* c = block->first_child; c; c = c->next_sibling) {
        bool foldable = false;
        if (c->kind == NODE_FIELD) foldable = field_is_const(c, ctx->reg, opt, inames, ic);
        else if (c->kind == NODE_BLOCK) foldable = (!c->param && block_is_const(c, ctx->reg, opt, inames, ic));
        if (foldable) {
            if (!in_block) {
                uid = next_uid(ctx->c); bi = 0; sz = 0; pl = 0;
                emitf(cc, "%sstatic const uint8_t __const_%d[] = {\n", indent, uid);
                in_block = true;
            }
            if (c->kind == NODE_BLOCK)
                emit_const_block(cc, c, ctx->reg, indent, &bi, &pl, opt, inames, ic, ctx->root, &anchors);
            else if (is_declared_type(ctx->reg, c->basetype))
                emit_const_declared(cc, c, ctx->reg, indent, &bi, &pl, opt, inames, ic, ctx->root, &anchors);
            else
                emit_const_field(cc, c, indent, &bi, &pl, opt, inames, ic, ctx->root, &anchors);
            sz = bi;
        } else {
            if (in_block) {
                emitf(cc, "\n%s};\n%s/* const block: %d bytes */\n%sbytelizer_put_bytes(%s, __const_%d, %d);\n",
                        indent, indent, sz, indent, ctx_var, uid, sz);
                if (opt) emit_anchor_patches(cc, ctx_var, indent, sz, &anchors);
                in_block = false;
                anchors.count = 0;
            }
            if (c->kind == NODE_FIELD) emit_field(cc, c, ctx->reg, ctx_var, indent, depth, dir);
            else emit_body(cc, c, ctx->reg, ctx_var, depth + 1, dir);
        }
    }
    if (in_block) {
        emitf(cc, "\n%s};\n%s/* const block: %d bytes */\n%sbytelizer_put_bytes(%s, __const_%d, %d);\n",
                indent, indent, sz, indent, ctx_var, uid, sz);
        if (opt) emit_anchor_patches(cc, ctx_var, indent, sz, &anchors);
    }
    return true;
}
