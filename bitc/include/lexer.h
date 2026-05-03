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

#ifndef _BITS_LEXER_H_
#define _BITS_LEXER_H_

#include <stdbool.h>
#include <bitc/token.h>

typedef struct {
    char*    filename;
    char*    source;
    int      pos;
    int      len;
    int      line;
    int      col;
    token_t  current;
    token_t  peek;
    bool     has_peek;
} lexer_t;

void lexer_open(lexer_t* l, const char* filename);
void lexer_close(lexer_t* l);
void lexer_next(lexer_t* l);
token_t lexer_peek_token(lexer_t* l);

bool lexer_match(lexer_t* l, token_type_t t);
bool lexer_match_ident(lexer_t* l, const char* s);
void lexer_expect(lexer_t* l, token_type_t t);

#endif /* _BITS_LEXER_H_ */
