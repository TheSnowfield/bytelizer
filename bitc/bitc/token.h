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

#ifndef _BITC_TOKEN_H_
#define _BITC_TOKEN_H_

typedef enum {
  TOK_EOF = 0,
  TOK_IDENT,
  TOK_NUMBER,
  TOK_STRING,
  TOK_LBRACE,        /* { */
  TOK_RBRACE,        /* } */
  TOK_LBRACKET,      /* [ */
  TOK_RBRACKET,      /* ] */
  TOK_SEMICOLON,     /* ; */
  TOK_COLON,         /* : */
  TOK_COMMA,         /* , */
  TOK_LT,            /* < */
  TOK_GT,            /* > */
  TOK_HASH,          /* # */
  TOK_SLASH,         /* / */
  TOK_DOT,           /* . */
  TOK_LPAREN,        /* ( */
  TOK_RPAREN,        /* ) */
  TOK_STAR,          /* * */
  TOK_MINUS,         /* - */
  TOK_ARROW,         /* -> */
  TOK_DOLLAR,        /* $ */
} token_type_t;

static inline const char* tok_name(token_type_t t) {
  switch (t) {
    case TOK_EOF:       return "EOF";
    case TOK_IDENT:     return "ident";
    case TOK_NUMBER:    return "number";
    case TOK_STRING:    return "string";
    case TOK_LBRACE:    return "{";
    case TOK_RBRACE:    return "}";
    case TOK_LBRACKET:  return "[";
    case TOK_RBRACKET:  return "]";
    case TOK_SEMICOLON: return ";";
    case TOK_COLON:     return ":";
    case TOK_COMMA:     return ",";
    case TOK_LT:        return "<";
    case TOK_GT:        return ">";
    case TOK_HASH:      return "#";
    case TOK_SLASH:     return "/";
    case TOK_DOT:       return ".";
    case TOK_LPAREN:    return "(";
    case TOK_RPAREN:    return ")";
    case TOK_STAR:      return "*";
    case TOK_MINUS:     return "-";
    case TOK_ARROW:     return "->";
    case TOK_DOLLAR:    return "$";
    default:            return "?";
  }
}

typedef struct {
    token_type_t type;
    char* text;
    int line;
    int col;
} token_t;

#endif /* _BITC_TOKEN_H_ */
