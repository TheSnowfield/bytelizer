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
#include <stdlib.h>
#include <bitc/ast.h>
#include <bitc/pass.h>
#include <bitc/codegen_types.h>

void collect_types(node_t* n, type_registry_t* reg) {
    if (n->kind == NODE_DECLARE && n->name) {
        reg->count++;
        reg->nodes = (node_t**)realloc(reg->nodes, reg->count * sizeof(node_t*));
        reg->nodes[reg->count - 1] = n;
    }
    for (node_t* c = n->first_child; c; c = c->next_sibling)
        collect_types(c, reg);
}

node_t* find_declared_type(type_registry_t* reg, const char* name) {
    if (!name) return NULL;
    for (int i = 0; i < reg->count; i++)
        if (reg->nodes[i]->name && strcmp(reg->nodes[i]->name, name) == 0)
            return reg->nodes[i];
    return NULL;
}

bool is_declared_type(type_registry_t* reg, const char* name) {
    return find_declared_type(reg, name) != NULL;
}