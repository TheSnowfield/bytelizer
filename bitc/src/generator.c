// SPDX-License-Identifier: GPL-2.0+
/*******************************************************************************
 * This file is the part of the Bytelizer library
 *
 * (C) Copyright 2026 TheSnowfield
 * (C) Copyright 2026 DeepSeek V4 Pro
 *
 * Authors: TheSnowfield <17957399+TheSnowfield@users.noreply.github.com>
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>

#include <bitc/bitc.h>
#include <bitc/const.h>
#include <bitc/ast.h>
#include <bitc/pass.h>
#include <bitc/compiler.h>
#include <bitc/utils.h>
#include <bitc/codegen_plugin.h>
#include <bitc/codegen_mapping.h>
#include <bitc/codegen_types.h>

/* ── forward declarations ── */
static void generate_body(compiler_t* cc, node_t* block, type_registry_t* reg,
                          const char* ctx_var, int depth, direction_t dir);
static void generate_block_full(compiler_t* cc, node_t* block, type_registry_t* reg,
                                const char* ctx_var, int depth, direction_t dir);
static void generate_field(compiler_t* cc, node_t* f, type_registry_t* reg,
                           const char* ctx_var, const char* indent, int depth, direction_t dir);
static int  field_path_roots(node_t* block, char roots[][64], int max);
static const char* field_eff_name(node_t* f);

/* ================================================================
 *  CODE GENERATOR
 * ================================================================ */

/* ── block meta helpers ── */

static const char* block_get_name(node_t* b) {
  for (int i = 0; i < b->attr_count; i++)
    if (strncmp(b->attrs[i], "name:", 5) == 0)
      return b->attrs[i] + 5;
  return NULL;
}

static direction_t block_get_mode(node_t* b) {
  for (int i = 0; i < b->attr_count; i++)
    if (strcmp(b->attrs[i], "mode:unpack") == 0)
      return DIR_UNPACK;
  return DIR_PACK;
}

/* resolve $ref:NAME */
static const char* resolve_ref(node_t* root, const char* name,
                                char input_names[][64], int input_count) {
  for (int i = 0; i < input_count; i++)
    if (strcmp(input_names[i], name) == 0) return name;
  const char* sep = strstr(name, "->");
  if (!sep) sep = strchr(name, '.');
  if (sep) {
    int root_len = (int)(sep - name);
    for (int i = 0; i < input_count; i++)
      if (strncmp(input_names[i], name, root_len) == 0 && input_names[i][root_len] == '\0')
        return name;
  }
  return find_attr_value(root, name);
}

/* convert "session.timestamp" → "session->timestamp" */
static void field_deref(const char* name, char* out, size_t outsz) {
    const char* dot = strchr(name, '.');
    if (!dot) { snprintf(out, outsz, "%s", name); return; }
    memcpy(out, name, (size_t)(dot - name));
    out += (dot - name);
    const char* rest = dot + 1;
    while (rest && *rest) {
        const char* next = strchr(rest, '.');
        int len = next ? (int)(next - rest) : (int)strlen(rest);
        out += snprintf(out, outsz, "->%.*s", len, rest);
        rest = next ? next + 1 : NULL;
    }
}

/* get effective field name: f->name or $ref:NAME → NAME */
static const char* field_eff_name(node_t* f) {
  if (f->name) return f->name;
  if (f->value && strncmp(f->value, "$ref:", 5) == 0) return f->value + 5;
  return NULL;
}

static int field_path_roots(node_t* block, char roots[][64], int max) {
  int n = 0;
  for (node_t* c = block->first_child; c; c = c->next_sibling) {
    const char* eff = field_eff_name(c);
    if (!eff) continue;
    const char* sep = strchr(eff, '.');
    if (!sep) sep = strstr(eff, "->");
    if (!sep) continue;
    int len = (int)(sep - eff);
    bool found = false;
    for (int i = 0; i < n; i++)
      if (strncmp(roots[i], eff, len) == 0 && roots[i][len] == '\0')
        { found = true; break; }
    if (!found && n < max) {
      memcpy(roots[n], eff, len);
      roots[n][len] = '\0';
      n++;
    }
  }
  return n;
}

/* ── generate pack and/or unpack functions ── */
static void generate_block_function(compiler_t* cc, node_t* block, type_registry_t* reg, node_t* root) {
  if (block->kind != NODE_BLOCK) return;

  char input_types[16][64];
  cc->input_count = 0;
  for (node_t* c = root->first_child; c; c = c->next_sibling) {
    if (c->kind != NODE_INCLUDE || !c->value) continue;
    if (strncmp(c->value, "input:", 6) != 0) continue;
    const char* v = c->value + 6;
    while (*v == ' ') v++;
    const char* lt = strchr(v, '<');
    const char* gt = lt ? strchr(lt+1, '>') : NULL;
    const char* type_start, * type_end, * name_start;
    if (lt && gt) {
      type_start = lt + 1; type_end = gt; name_start = gt + 1;
    } else {
      type_start = v;
      const char* sp = strchr(v, ' ');
      if (!sp) continue;
      type_end = sp; name_start = sp + 1;
    }
    while (*name_start == ' ' || *name_start == '*') name_start++;
    if (cc->input_count < 16) {
      int tl = (int)(type_end - type_start);
      if (tl > 63) tl = 63;
      memcpy(input_types[cc->input_count], type_start, tl);
      input_types[cc->input_count][tl] = '\0';
      snprintf(cc->input_names[cc->input_count], sizeof(cc->input_names[0]), "%s", name_start);
      cc->input_count++;
    }
  }

  const char* name_tmpl = block_get_name(block);
  if (!name_tmpl) return;

  for (node_t* c = block->first_child; c; c = c->next_sibling) {
      if (c->kind != NODE_FIELD) continue;
      if (!c->name || c->value) continue;
      if (strchr(c->name, '.') || strstr(c->name, "->")) continue;
      if (is_declared_type(reg, c->basetype)) continue;
      bool found = false;
      for (int j = 0; j < cc->input_count; j++)
          if (strcmp(cc->input_names[j], c->name) == 0) { found = true; break; }
      if (!found)
          die("bitc: error: line %d: field '%s' has no [input:] declaration", c->line, c->name);
  }

  char* func_prefix = resolve_name_tmpl(root, name_tmpl);
  direction_t dir = block_get_mode(block);
  bool to_svc = (dir == DIR_PACK);
  bool from_svc = (dir == DIR_UNPACK);

  if (to_svc) {
      emitf(cc, "static inline void %s(bytelizer_ctx_t* ctx", func_prefix);
      for (node_t* c = block->first_child; c; c = c->next_sibling) {
          if (c->kind != NODE_FIELD) continue;
          if (!c->name || c->value) continue;
          if (strchr(c->name, '.') || strstr(c->name, "->")) continue;
          const char* put = get_put_function_for_type(c->basetype);
          if (!put) continue;
          if (strcmp(c->basetype, BIT_STRING) == 0)
              emitf(cc, ", const char* %s", c->name);
          else {
              int bits = field_base_size(c) * 8;
              emitf(cc, ", uint%d_t %s", bits, c->name);
          }
      }
      for (int j = 0; j < cc->input_count; j++)
          emitf(cc, ", %s %s", input_types[j], cc->input_names[j]);
      emitf(cc, ")\n{\n");
      generate_block_full(cc, block, reg, "ctx", 0, DIR_PACK);
      emitf(cc, "}\n\n");
  }

  if (from_svc) {
    char roots[16][64];
    int nroot = field_path_roots(block, roots, 16);

    emitf(cc, "static inline int %s(bytelizer_ctx_t* ctx", func_prefix);
    for (int i = 0; i < nroot; i++) {
      const char* type = "void*";
      for (int j = 0; j < cc->input_count; j++)
        if (strcmp(cc->input_names[j], roots[i]) == 0)
          { type = input_types[j]; break; }
      emitf(cc, ", %s %s", type, roots[i]);
    }
    for (node_t* c = block->first_child; c; c = c->next_sibling) {
      if (c->kind != NODE_FIELD) continue;
      const char* eff = field_eff_name(c);
      if (!eff) continue;
      if (strchr(eff, '.') || strstr(eff, "->")) continue;
      if (c->value && strncmp(c->value, "$ref:", 5) != 0) continue;
      const char* get = field_get_for(c);
      if (!get) continue;
      if (strcmp(c->basetype, BIT_STRING) == 0)
        emitf(cc, ", const char* %s", eff);
      else {
        int bits = field_base_size(c) * 8;
        emitf(cc, ", uint%d_t %s", bits, eff);
      }
    }
    emitf(cc, ")\n{\n");
    generate_block_full(cc, block, reg, "ctx", 0, DIR_UNPACK);
    emitf(cc, "  return 0;\n");
    emitf(cc, "}\n\n");
  }
  free(func_prefix);
}

/* ── generate one field ── */
static void generate_field(compiler_t* cc, node_t* f, type_registry_t* reg,
                       const char* ctx_var, const char* indent, int depth, direction_t dir) {
    for (int ai = 0; ai < f->attr_count; ai++) {
        const char* a = f->attrs[ai];
        if (!strstr(a, "with-uint") && !strstr(a, "input:") && !strstr(a, "name:")
            && !strstr(a, "mode:") && !strstr(a, "SSO_"))
            fprintf(stderr, "bitc: warning: line %d: unknown attr '%s' on field '%s'\n",
                    f->line, a, f->name ? f->name : f->basetype);
    }

    if (dir == DIR_PACK) {
        const char* put = field_put_for(f);
        if (!put && is_declared_type(reg, f->basetype)) {
            node_t* decl = find_declared_type(reg, f->basetype);
            emitf(cc, "%s/* %s %s */\n", indent, f->basetype, f->name ? f->name : "");

            const char* outer_pfx = NULL;
            int outer_bid = -1;
            if (decl && decl->first_child) {
                for (int ai = 0; ai < f->attr_count; ai++)
                    if (attr_is_prefix(f->attrs[ai])) { outer_pfx = attr_to_prefix(f->attrs[ai]); break; }
                if (outer_pfx) {
                    outer_bid = next_uid(cc);
                    emitf(cc, "%s/* [%s] */\n", indent, outer_pfx);
                    emitf(cc, "%sbytelizer_barrier_enter(b%d, %s, %s);\n", indent, outer_bid, ctx_var, outer_pfx);
                }
                int matched[64] = {0};
                for (int ci = 0; ci < 64; ci++) matched[ci] = 0;
                int child_count = 0;
                for (node_t* of = f->first_child; of; of = of->next_sibling, child_count++)
                    if (of->name) { /* count */ }
                for (node_t* tf = decl->first_child; tf; tf = tf->next_sibling) {
                    if (tf->kind != NODE_FIELD) continue;
                    node_t* ov = NULL;
                    int _ci = 0;
                    for (node_t* of = f->first_child; of; of = of->next_sibling, _ci++) {
                        const char* ofname = of->name ? of->name : of->basetype;
                        if (tf->name && ofname && strcmp(tf->name, ofname) == 0)
                            { ov = of; matched[_ci] = 1; break; }
                    }
                    if (ov) {
                        for (int ai = 0; ai < tf->attr_count; ai++)
                            node_add_attr(ov, tf->attrs[ai]);
                        if (!ov->name && ov->basetype) {
                            ov->name = ov->basetype;
                            ov->basetype = xstrdup(tf->basetype);
                        }
                    }
                    node_t* gen = ov ? ov : tf;
                    if (!ov && tf->is_default)
                        fprintf(stderr, "bitc: warning: line %d: field '%s' in '%s' of type '%s' uses unset default 0\n",
                                f->line, tf->name, f->name, f->basetype);
                    if (tf->basetype && strcmp(tf->basetype, BIT_BYTES) == 0) {
                        const char* pfx = NULL;
                        for (int ai = 0; ai < gen->attr_count; ai++)
                            if (attr_is_prefix(gen->attrs[ai])) { pfx = attr_to_prefix(gen->attrs[ai]); break; }
                        if (pfx) {
                            int uid = next_uid(cc);
                            emitf(cc, "%s/* [%s] */\n", indent, pfx);
                            emitf(cc, "%sbytelizer_barrier_enter(b%d, %s, %s);\n", indent, uid, ctx_var, pfx);
                            if (gen->first_child)
                                generate_body(cc, gen, reg, ctx_var, depth+1, dir);
                            emitf(cc, "%sbytelizer_barrier_leave(b%d);\n", indent, uid);
                        } else if (gen->first_child) {
                            generate_body(cc, gen, reg, ctx_var, depth, dir);
                        }
                    } else {
                        if (gen->value && !gen->first_child) {
                            const char* put_t = field_put_for(gen);
                            if (!put_t) put_t = get_put_function_for_type(gen->basetype);
                            if (put_t) {
                                if (strncmp(gen->value, "$ref:", 5) == 0) {
                                    const char* rv = resolve_ref(cc->root, gen->value+5,
                                                                 cc->input_names, cc->input_count);
                                    emitf(cc, "%s%s(%s, %s);\n", indent, put_t, ctx_var, rv ? rv : "0");
                                } else {
                                    emitf(cc, "%s%s(%s, %s);\n", indent, put_t, ctx_var, gen->value);
                                }
                            }
                        } else {
                            generate_field(cc, gen, reg, ctx_var, indent, depth, dir);
                        }
                    }
                }
                int ci = 0;
                for (node_t* of = f->first_child; of; of = of->next_sibling, ci++)
                    if (!matched[ci] && of->kind == NODE_FIELD && (of->name || of->basetype))
                        fprintf(stderr, "bitc: warning: line %d: field '%s' in '%s' has no matching field in type '%s'\n",
                                of->line, of->name ? of->name : "(anonymous)", f->name, f->basetype);
            }
            if (outer_pfx)
                emitf(cc, "%sbytelizer_barrier_leave(b%d);\n", indent, outer_bid);
            return;
        }
        if (!put) {
            fprintf(stderr, "bitc: error: line %d: unknown type '%s'\n", f->line, f->basetype);
            cc->errors++;
            return;
        }
        if (f->name && !f->value) {
            emitf(cc, "%s%s(%s, %s);\n", indent, put, ctx_var, f->name);
        } else if (f->value && !f->name) {
            if (strncmp(f->value, "$ref:", 5) == 0) {
                const char* val = resolve_ref(cc->root, f->value + 5,
                                              cc->input_names, cc->input_count);
                if (val) {
                    if (strcmp(val, f->value + 5) == 0)
                        emitf(cc, "%s%s(%s, %s);\n", indent, put, ctx_var, val);
                    else if (strcmp(f->basetype, BIT_STRING) == 0)
                        emitf(cc, "%s%s(%s, \"%s\");\n", indent, put, ctx_var, val);
                    else
                        emitf(cc, "%s%s(%s, %s);\n", indent, put, ctx_var, val);
                } else {
                    emitf(cc, "%s/* $ref:%s not found */\n", indent, f->value + 5);
                }
            } else if (strcmp(f->basetype, BIT_STRING) == 0) {
                emitf(cc, "%s%s(%s, \"%s\");\n", indent, put, ctx_var, f->value);
            } else {
                char* v = xstrdup(f->value);
                char* tok = strtok(v, " ");
                while (tok) {
                    emitf(cc, "%s%s(%s, %s);\n", indent, put, ctx_var, tok);
                    tok = strtok(NULL, " ");
                }
                free(v);
            }
        } else if (f->name && f->value) {
            emitf(cc, "%s/* %s %s = %s */\n", indent, f->basetype, f->name, f->value);
            emitf(cc, "%s%s(%s, %s);\n", indent, put, ctx_var, f->name);
        }
        return;
    }

    /* DIR_UNPACK */
    const char* get = field_get_for(f);
    if (!get && is_declared_type(reg, f->basetype)) {
        emitf(cc, "%s/* unpack %s %s */\n", indent, f->basetype, f->name ? f->name : "anon");
        if (f->first_child) {
            for (node_t* fc = f->first_child; fc; fc = fc->next_sibling) {
                if (fc->kind == NODE_FIELD)
                    generate_field(cc, fc, reg, ctx_var, indent, depth + 1, dir);
                else if (fc->kind == NODE_BLOCK)
                    generate_body(cc, fc, reg, ctx_var, depth + 1, dir);
            }
        }
        return;
    }
    if (!get) {
        fprintf(stderr, "bitc: error: line %d: unknown type '%s'\n", f->line, f->basetype);
        cc->errors++;
        return;
    }

    int bits = field_base_size(f) * 8;

    const char* eff_name = f->name;
    if (!eff_name && f->value && strncmp(f->value, "$ref:", 5) == 0)
        eff_name = f->value + 5;

    if (!eff_name && !f->value) {
        emitf(cc, "%sbytelizer_skip(%s, %d);\n", indent, ctx_var, bits / 8);
        return;
    }
    if (f->value && !eff_name) {
        if (strchr(f->value, ' ')) {
            char* dup = xstrdup(f->value);
            char* tok = strtok(dup, " ");
            while (tok) {
                int uid = next_uid(cc);
                emitf(cc, "%suint%d_t __chk%d; %s(%s, &__chk%d);\n",
                      indent, bits, uid, get, ctx_var, uid);
                tok = strtok(NULL, " ");
            }
            free(dup);
        } else {
            int uid = next_uid(cc);
            emitf(cc, "%suint%d_t __chk%d; %s(%s, &__chk%d); /* %s */\n",
                  indent, bits, uid, get, ctx_var, uid, f->value);
        }
        return;
    }
    char deref[256];
    field_deref(eff_name, deref, sizeof(deref));
    if (eff_name && !f->value) {
        emitf(cc, "%s%s(%s, &%s);\n", indent, get, ctx_var, deref);
    } else if (eff_name && f->value) {
        emitf(cc, "%s/* unpack %s %s = %s */\n", indent, f->basetype, eff_name, f->value);
        emitf(cc, "%s%s(%s, &%s);\n", indent, get, ctx_var, deref);
    }
}

/* ── generate block body ── */
static void generate_body(compiler_t* cc, node_t* block, type_registry_t* reg,
                     const char* ctx_var, int depth, direction_t dir) {
    char ib[64];
    int n = (depth + 1) * 2; if (n > 60) n = 60;
    memset(ib, ' ', n); ib[n] = '\0';
    const char* in = ib;

    if (cc->optimize && (dir == DIR_PACK || dir == DIR_UNPACK)) {
        pass_ctx_t pctx;
        memset(&pctx, 0, sizeof(pctx));
        pctx.c = cc;
        pctx.root = cc->root;
        pctx.reg = reg;
        pctx.input_names = cc->input_names;
        pctx.input_count = cc->input_count;
        pctx.optimize = cc->optimize;
        if (dir == DIR_PACK) {
            if (pass1_const_fold(cc, block, &pctx, ctx_var, in, depth, generate_field, generate_body, dir))
                return;
        } else {
            if (pass2_skip_merge(cc, block, &pctx, ctx_var, in, depth, generate_field, generate_body, dir))
                return;
        }
    }

    for (node_t* c = block->first_child; c; c = c->next_sibling) {
        if (c->kind == NODE_FIELD) {
            generate_field(cc, c, reg, ctx_var, in, depth, dir);
        } else if (c->kind == NODE_BLOCK) {
            if (c->param && dir == DIR_PACK) {
                int xid = next_uid(cc);
                const char* xname = c->name ? c->name : "anon";
                const char* xpfx = NULL;
                for (int i = 0; i < c->attr_count; i++)
                    if (attr_is_prefix(c->attrs[i])) { xpfx = attr_to_prefix(c->attrs[i]); break; }
                int bid = xpfx ? next_uid(cc) : -1;
                if (xpfx) {
                    emitf(cc, "%s/* [%s] */\n", in, xpfx);
                    emitf(cc, "%sbytelizer_barrier_enter(b%d, %s, %s);\n", in, bid, ctx_var, xpfx);
                }
                emitf(cc, "%s/* transform: %s(%s) */\n", in, xname, c->param);
                emitf(cc, "%sbytelizer_alloc(_bx%d, 512);\n", in, xid);
                char bx[32]; snprintf(bx, sizeof(bx), "_bx%d", xid);
                generate_body(cc, c, reg, bx, depth + 1, dir);
                emitf(cc, "%sbitc_xform_t* _xf%d = bitc_xform_lookup(\"%s\");\n", in, xid, xname);
                emitf(cc, "%sif (_xf%d && _xf%d->apply_pack) {\n", in, xid, xid);
                emitf(cc, "%s  _xf%d->apply_pack(%s, _bx%d, \"%s\");\n", in, xid, ctx_var, xid, c->param);
                emitf(cc, "%s} else {\n", in);
                emitf(cc, "%s  bytelizer_put_bytelizer(%s, _bx%d);\n", in, ctx_var, xid);
                emitf(cc, "%s}\n", in);
                emitf(cc, "%sbytelizer_destroy(_bx%d);\n", in, xid);
                if (xpfx) emitf(cc, "%sbytelizer_barrier_leave(b%d);\n", in, bid);
            } else if (c->param && dir == DIR_UNPACK) {
                int xid = next_uid(cc);
                const char* xname = c->name ? c->name : "anon";
                emitf(cc, "%s/* unpack transform: %s(%s) */\n", in, xname, c->param);
                emitf(cc, "%sbytelizer_alloc(_bx%d, 512);\n", in, xid);
                emitf(cc, "%sbitc_xform_t* _xf%d = bitc_xform_lookup(\"%s\");\n", in, xid, xname);
                emitf(cc, "%sif (_xf%d && _xf%d->apply_unpack) {\n", in, xid, xid);
                emitf(cc, "%s  _xf%d->apply_unpack(_bx%d, %s, \"%s\");\n", in, xid, xid, ctx_var, c->param);
                emitf(cc, "%s} else {\n", in);
                emitf(cc, "%s  bytelizer_put_bytelizer(_bx%d, %s);\n", in, xid, ctx_var);
                emitf(cc, "%s}\n", in);
                char bx[32]; snprintf(bx, sizeof(bx), "_bx%d", xid);
                generate_body(cc, c, reg, bx, depth + 1, dir);
                emitf(cc, "%sbytelizer_destroy(_bx%d);\n", in, xid);
            } else {
                if (c->name) emitf(cc, "%s/* nested block: %s */\n", in, c->name);
                else emitf(cc, "%s/* nested bytes */\n", in);
                generate_block_full(cc, c, reg, ctx_var, depth + 1, dir);
            }
        }
    }
}

/* ── generate full block with barrier handling ── */
static void generate_block_full(compiler_t* cc, node_t* block, type_registry_t* reg,
                            const char* ctx_var, int depth, direction_t dir) {
    char ib[64];
    int n = (depth + 1) * 2; if (n > 60) n = 60;
    memset(ib, ' ', n); ib[n] = '\0';
    const char* in = ib;
    if (block->kind != NODE_BLOCK) return;

    const char* pfx = NULL;
    for (int i = 0; i < block->attr_count; i++)
        if (attr_is_prefix(block->attrs[i])) { pfx = attr_to_prefix(block->attrs[i]); break; }

    if (pfx && dir == DIR_PACK) {
        int uid = next_uid(cc);
        emitf(cc, "%s/* [%s] */\n", in, pfx);
        emitf(cc, "%sbytelizer_barrier_enter(b%d, %s, %s);\n", in, uid, ctx_var, pfx);
        generate_body(cc, block, reg, ctx_var, depth, dir);
        emitf(cc, "%sbytelizer_barrier_leave(b%d);\n", in, uid);
    } else if (pfx && dir == DIR_UNPACK) {
        int uid = next_uid(cc);
        emitf(cc, "%s/* [%s] -- skip length prefix */\n", in, pfx);
        int pbits = strstr(pfx, BIT_UINT16) ? 16 : 32;
        emitf(cc, "%suint%d_t __pflen%d; bytelizer_get_uint%d_be(%s, &__pflen%d);\n",
                in, pbits, uid, pbits, ctx_var, uid);
        generate_body(cc, block, reg, ctx_var, depth, dir);
    } else {
        generate_body(cc, block, reg, ctx_var, depth, dir);
    }
}

/* ── emit unknown attrs ── */
static void emit_unknown_walk(compiler_t* cc, node_t* n, bool* first, plugin_ctx_t* plg) {
  if (!n) return;
  if (n->kind == NODE_UNKNOWN_ATTR && n->value) {
    if (cc) {
      if (*first) { emitf(cc, "\n/* ── unhandled attrs ── */\n"); *first = false; }
      emitf(cc, "// bitc:attr %s\n", n->value);
    }
    if (plg) plugin_feed(plg, n->value);
  }
  for (node_t* ch = n->first_child; ch; ch = ch->next_sibling)
    emit_unknown_walk(cc, ch, first, plg);
}

static void emit_unknown_attrs(compiler_t* cc, node_t* root, plugin_ctx_t* plg) {
  bool first = true;
  emit_unknown_walk(cc, root, &first, plg);
}

/* ── main codegen entry ── */
void gen_c(compiler_t* cc, node_t* root, const char* source_name, const char* plugin_path) {
    (void)cc->argc; (void)cc->argv;
    type_registry_t reg = {0};
    collect_types(root, &reg);
    cc->root = root;

    emitf(cc, "/* @brief Compile-time built BIT template file\n");
    emitf(cc, " * @name %s.bit\n", source_name);
    if (cc->argc > 0) {
        emitf(cc, " * @cmdline \"%s", cc->argv[0]);
        for (int i = 1; i < cc->argc; i++)
            emitf(cc, " \\%s\n *     %s", "", cc->argv[i]);
        emitf(cc, "\"\n");
    }
    emitf(cc, " */\n\n");

    char guard[256];
    str_toupper(source_name, guard, sizeof(guard));

    emitf(cc, "#ifndef _BYTELIZER_BIT_COMPILED_%s_H_\n", guard);
    emitf(cc, "#define _BYTELIZER_BIT_COMPILED_%s_H_\n\n", guard);
    emitf(cc, "#include <string.h>\n");
    emitf(cc, "#include <bytelizer/codec.h>\n");
    emitf(cc, "#include <bytelizer/advanced.h>\n");
    emitf(cc, "#include <bytelizer/barrier.h>\n\n");
    emitf(cc, "/* ── transform function registry ── */\n");
    emitf(cc, "typedef struct {\n");
    emitf(cc, "  const char* name;\n");
    emitf(cc, "  void (*apply_pack)(bytelizer_ctx_t* parent, bytelizer_ctx_t* child, const char* params);\n");
    emitf(cc, "  void (*apply_unpack)(bytelizer_ctx_t* child, bytelizer_ctx_t* parent, const char* params);\n");
    emitf(cc, "} bitc_xform_t;\n\n");
    emitf(cc, "extern bitc_xform_t* bitc_xform_lookup(const char* name);\n\n");

    plugin_ctx_t plg;
    if (plugin_path && plugin_path[0])
        plugin_start(&plg, plugin_path, source_name);

    for (node_t* c = root->first_child; c; c = c->next_sibling) {
        if (c->kind == NODE_BLOCK)
            generate_block_function(cc, c, &reg, root);
    }

    emit_unknown_attrs(cc, root, plugin_path ? &plg : NULL);

    if (plugin_path && plugin_path[0])
        plugin_stop(&plg, cc->out, plugin_path);

    emitf(cc, "#endif /* _BYTELIZER_BIT_COMPILED_%s_H_ */\n", guard);
    free(reg.nodes);
}
