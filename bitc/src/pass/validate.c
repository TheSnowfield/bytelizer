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
#include <ctype.h>
#include <bitc/bitc.h>
#include <bitc/const.h>
#include <bitc/ast.h>
#include <bitc/compiler.h>
#include <bitc/pass.h>
#include <bitc/utils.h>

static bool name_is_valid(const char* name, int line) {
    if (!name || !*name) return true;
    if (isdigit((unsigned char)name[0])) {
        fprintf(stderr, "bitc: error: line %d: name '%s' starts with digit\n", line, name);
        return false;
    }
    for (const char* p = name; *p; p++) {
        if (!isalnum((unsigned char)*p) && *p != '_' && *p != '-' && *p != '.' && *p != '<' && *p != '>') {
            fprintf(stderr, "bitc: error: line %d: name '%s' contains invalid char '%c'\n", line, name, *p);
            return false;
        }
    }
    return true;
}

static void pass0_walk(node_t* n, node_t* root, char input_names[][64], int input_count) {
    if (!n) return;
    /* expand $fmt{KEY} in node name */
    if (n->name && strstr(n->name, "$fmt{")) {
        char* resolved = resolve_name_tmpl(root, n->name);
        free(n->name);
        n->name = resolved;
    }
    /* expand $fmt{KEY} in value */
    if (n->value && strstr(n->value, "$fmt{")) {
        char* resolved = resolve_name_tmpl(root, n->value);
        free(n->value);
        n->value = resolved;
    }
    /* expand ${KEY} in value (standalone attr ref) */
    if (n->value && n->value[0] == '$' && n->value[1] == '{') {
        const char* end = strchr(n->value + 2, '}');
        if (end) {
            char key[256]; int kl = (int)(end - (n->value + 2));
            if (kl > 255) kl = 255;
            memcpy(key, n->value + 2, kl); key[kl] = '\0';
            const char* val = find_attr_value(root, key);
            if (val) {
                free(n->value);
                n->value = xstrdup(val);
            }
        }
    }
    /* resolve $ref:KEY */
    if (n->kind == NODE_FIELD && n->value && strncmp(n->value, "$ref:", 5) == 0) {
        const char* name = n->value + 5;
        if (!is_input_ref(name, input_names, input_count)) {
            const char* aval = find_attr_value(root, name);
            if (aval) {
                free(n->value);
                n->value = xstrdup(aval);
            }
        }
    }
    /* recurse */
    for (node_t* c = n->first_child; c; c = c->next_sibling)
        pass0_walk(c, root, input_names, input_count);
}

void pass0_resolve(node_t* root, char input_names[][64], int input_count) {
    if (!root) return;
    for (node_t* c = root->first_child; c; c = c->next_sibling)
        pass0_walk(c, root, input_names, input_count);
}

static void validate_field_type(node_t* f, compiler_t* cc, node_t* parent) {
    if (is_known_basetype(f->basetype)) return;
    if (compiler_is_declared(cc, f->basetype)) {
        if (f->value) {
            fprintf(stderr, "bitc: error: line %d: type instance '%s' cannot have a value '%s'; use '%s;' or '%s { ... }'\n",
                    f->line, f->basetype, f->value, f->basetype, f->basetype);
            cc->errors++;
        }
        return;
    }
    if (parent && parent->basetype && compiler_is_declared(cc, parent->basetype)) {
        node_t* decl = compiler_find_declared(cc, parent->basetype);
        if (decl) {
            for (node_t* tf = decl->first_child; tf; tf = tf->next_sibling)
                if (tf->name && strcmp(tf->name, f->basetype) == 0) return;
        }
    }
    if (f->value)
        fprintf(stderr, "bitc: error: line %d: type '%s' cannot have a value; use '%s;' or '%s { ... }'\n",
                f->line, f->basetype, f->basetype, f->basetype);
    else
        fprintf(stderr, "bitc: error: line %d: unknown type '%s'\n", f->line, f->basetype);
    cc->errors++;
}

static void validate_walk(node_t* n, compiler_t* cc) {
    if (!n) return;
    if (n->kind == NODE_DECLARE && n->name) {
        if (!name_is_valid(n->name, n->line)) cc->errors++;
    }
    if (n->kind == NODE_FIELD && n->basetype) {
        if (n->name && !name_is_valid(n->name, n->line)) cc->errors++;
        validate_field_type(n, cc, n->parent);
    }
    for (node_t* c = n->first_child; c; c = c->next_sibling)
        validate_walk(c, cc);
}

void pass0_validate(compiler_t* cc) {
    if (!cc->root) return;
    for (node_t* n = cc->root->first_child; n; n = n->next_sibling)
        if (n->kind == NODE_DECLARE && n->name) {
            cc->reg.count++;
            cc->reg.nodes = (node_t**)realloc(cc->reg.nodes, cc->reg.count * sizeof(node_t*));
            cc->reg.nodes[cc->reg.count - 1] = n;
        }
    for (node_t* c = cc->root->first_child; c; c = c->next_sibling)
        validate_walk(c, cc);
    free(cc->reg.nodes);
    cc->reg.nodes = NULL;
    cc->reg.count = 0;
}
