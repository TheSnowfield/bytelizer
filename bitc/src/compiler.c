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
#include <string.h>
#include <bitc/bitc.h>
#include <bitc/compiler.h>

void default_emit(compiler_t* c, const char* fmt, va_list ap) {
    vfprintf(c->out, fmt, ap);
}

node_t* compiler_find_declared(compiler_t* c, const char* name) {
    if (!name) return NULL;
    for (int i = 0; i < c->reg.count; i++)
        if (c->reg.nodes[i]->name && strcmp(c->reg.nodes[i]->name, name) == 0)
            return c->reg.nodes[i];
    return NULL;
}

bool compiler_is_declared(compiler_t* c, const char* name) {
    return compiler_find_declared(c, name) != NULL;
}

static const char* find_attr_dfs(node_t* n, const char* key, int klen) {
    if (!n) return NULL;
    if (n->value && strncmp(n->value, key, klen) == 0 && n->value[klen] == ':')
        return n->value + klen + 1;
    for (node_t* ch = n->first_child; ch; ch = ch->next_sibling) {
        const char* r = find_attr_dfs(ch, key, klen);
        if (r) return r;
    }
    return NULL;
}

const char* compiler_find_attr(compiler_t* c, const char* key) {
    if (!c->root || !key) return NULL;
    return find_attr_dfs(c->root, key, (int)strlen(key));
}
