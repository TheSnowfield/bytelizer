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

#ifndef _BITC_COMPILER_H_
#define _BITC_COMPILER_H_

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <bitc/ast.h>

typedef struct { node_t** nodes; int count; } type_registry_t;

typedef struct compiler_t compiler_t;
typedef void (*emit_fn)(compiler_t* c, const char* fmt, va_list ap);

struct compiler_t {
    /* output */
    FILE*   out;
    emit_fn emit;

    /* AST */
    node_t*       root;
    type_registry_t reg;

    /* input names: [input: <type> name] */
    char input_names[16][64];
    int  input_count;

    /* codegen state */
    int uid;
    char source_name[256];

    /* options */
    bool optimize;

    /* command line */
    int    argc;
    char** argv;

    /* error tracking */
    int errors;
};

/* ── emit helper ── */
static inline void emitf(compiler_t* c, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    c->emit(c, fmt, ap);
    va_end(ap);
}

static inline int next_uid(compiler_t* c) {
    return c->uid++;
}

void default_emit(compiler_t* c, const char* fmt, va_list ap);

/* ── utils ── */
node_t* compiler_find_declared(compiler_t* c, const char* name);
bool    compiler_is_declared(compiler_t* c, const char* name);
const char* compiler_find_attr(compiler_t* c, const char* key);

#endif /* _BITC_COMPILER_H_ */
