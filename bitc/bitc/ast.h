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

#ifndef _BITC_AST_H_
#define _BITC_AST_H_

#include <bitc/lexer.h>

typedef enum {
    NODE_PROGRAM,
    NODE_INCLUDE,
    NODE_DECLARE,
    NODE_BLOCK,
    NODE_FIELD,
    NODE_UNKNOWN_ATTR,
} node_kind_t;

static inline const char* node_kind_name(node_kind_t k) {
    switch (k) {
        case NODE_PROGRAM: return "program";
        case NODE_INCLUDE: return "include";
        case NODE_DECLARE: return "declare";
        case NODE_BLOCK:   return "block";
        case NODE_FIELD:   return "field";
        case NODE_UNKNOWN_ATTR: return "unknown_attr";
        default:           return "?";
    }
}

typedef struct node_t {
    node_kind_t  kind;
    char*        name;
    char*        value;
    char*        param;
    char*        basetype;
    char**       attrs;
    int          attr_count;
    int          line;           /* source line for plugin position tracking */
    bool         is_default;     /* value was auto-set to 0 in declare */
    struct node_t* parent;
    struct node_t* first_child;
    struct node_t* last_child;
    struct node_t* next_sibling;
} node_t;

node_t* node_new(node_kind_t kind);
void    node_add_child(node_t* parent, node_t* child);
void    node_add_attr(node_t* node, const char* attr);
void    node_print(node_t* n, int depth);
void    node_free(node_t* n);

typedef struct {
    lexer_t* lex;
    node_t*  root;
    char*    bits_dir;
    int      error_count;
    int      in_type_body;
} parser_t;

void parser_init(parser_t* p, lexer_t* lex, const char* bits_dir);
bool parser_parse(parser_t* p);
bool parser_include_file(parser_t* p, const char* rel_path);

#endif /* _BITC_AST_H_ */
