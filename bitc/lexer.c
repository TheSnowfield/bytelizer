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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <bitc/bitc.h>
#include <bitc/lexer.h>

static bool is_ident_start(int c) {
  return isalpha(c) || c == '_';
}
static bool is_ident_part(int c) {
    return isalnum(c) || c == '_';
}

static void lexer_fill(lexer_t* l, token_t* tk) {
  char* src = l->source;
  int p = l->pos;
  int end = l->len;

  /* skip whitespace & comments */
  while (p < end) {
    if (src[p] == '\n') { l->line++; l->col = 0; p++; continue; }
    if (isspace((unsigned char)src[p])) { l->col++; p++; continue; }
    if (src[p] == '/' && p+1 < end && src[p+1] == '/') {
      while (p < end && src[p] != '\n') p++;
      continue;
    }
    if (src[p] == '/' && p+1 < end && src[p+1] == '*') {
      p += 2; l->col += 2;
      while (p+1 < end && !(src[p] == '*' && src[p+1] == '/')) {
        if (src[p] == '\n') { l->line++; l->col = 0; } else l->col++;
        p++;
      }
      if (p+1 < end) { p += 2; l->col += 2; }
      continue;
    }
    break;
  }

  l->pos = p;
  if (p >= end) {
    tk->type = TOK_EOF;
    tk->text = xstrdup("");
    tk->line = l->line;
    tk->col  = l->col;
    return;
  }

  tk->line = l->line;
  tk->col  = l->col;

  char c = src[p];

  /* string literal */
  if (c == '"') {
    int start = p + 1;
    p++;
    while (p < end && src[p] != '"') {
      if (src[p] == '\\' && p+1 < end) p++;
      p++;
    }
    if (p >= end) die("%s:%d: unterminated string", l->filename, l->line);
    int slen = p - start;
    tk->text = (char*)malloc(slen + 1);
    if (!tk->text) die("oom");
    memcpy(tk->text, src + start, slen);
    tk->text[slen] = '\0';
    tk->type = TOK_STRING;
    p++; /* skip closing " */
    l->pos = p;
    l->col += (p - l->pos + slen);
    return;
  }

  /* number (hex or dec) */
  if ((c == '0' && p+1 < end && (src[p+1] == 'x' || src[p+1] == 'X')) || isdigit(c)) {
    int start = p;
    if (c == '0' && p+1 < end && (src[p+1] == 'x' || src[p+1] == 'X')) p += 2;
    while (p < end && (isxdigit((unsigned char)src[p]) || src[p] == 'x' || src[p] == 'X'))
      p++;
    int slen = p - start;
    tk->text = (char*)malloc(slen + 1);
    memcpy(tk->text, src + start, slen);
    tk->text[slen] = '\0';
    tk->type = TOK_NUMBER;
    l->pos = p;
    l->col += slen;
    return;
  }

  /* identifier */
  if (is_ident_start(c)) {
    int start = p;
    while (p < end && is_ident_part(src[p])) p++;
    int slen = p - start;
    tk->text = (char*)malloc(slen + 1);
    memcpy(tk->text, src + start, slen);
    tk->text[slen] = '\0';
    tk->type = TOK_IDENT;
    l->pos = p;
    l->col += slen;
    return;
  }

  /* single chars */
  p++;
  l->pos = p;
  l->col++;
  switch (c) {
    case '{': tk->type = TOK_LBRACE;   break;
    case '}': tk->type = TOK_RBRACE;   break;
    case '[': tk->type = TOK_LBRACKET; break;
    case ']': tk->type = TOK_RBRACKET; break;
    case ';': tk->type = TOK_SEMICOLON; break;
    case ':': tk->type = TOK_COLON;    break;
    case ',': tk->type = TOK_COMMA;    break;
    case '<': tk->type = TOK_LT;       break;
    case '>': tk->type = TOK_GT;       break;
    case '#': tk->type = TOK_HASH;     break;
    case '/': tk->type = TOK_SLASH;    break;
    case '.': tk->type = TOK_DOT;      break;
    case '(': tk->type = TOK_LPAREN;   break;
    case ')': tk->type = TOK_RPAREN;   break;
    case '*': tk->type = TOK_STAR;     break;
    case '$': tk->type = TOK_DOLLAR;   break;
    case '-':
      if (p < end && src[p] == '>') {
        p++; l->pos = p; l->col++;
        tk->type = TOK_ARROW;
      } else {
        tk->type = TOK_MINUS;
      }
      break;
    default:
      die("%s:%d: unexpected character '%c' (0x%02X)",
        l->filename, l->line, c, (unsigned char)c);
  }
  tk->text = (char*)malloc(4);
  if (tk->type == TOK_ARROW) {
    tk->text[0] = '-'; tk->text[1] = '>'; tk->text[2] = '\0';
  } else {
    tk->text[0] = c; tk->text[1] = '\0';
  }
}

void lexer_open(lexer_t* l, const char* filename) {
  memset(l, 0, sizeof(*l));
  l->filename = xstrdup(filename);
  size_t len;
  l->source = read_file(filename, &len);
  if (!l->source) die("cannot open '%s'", filename);
  l->len = len;
  l->line = 1;
  l->col  = 0;
  lexer_fill(l, &l->current);
}

void lexer_close(lexer_t* l) {
  free(l->filename);
  free(l->source);
  free(l->current.text);
  if (l->has_peek) free(l->peek.text);
}

void lexer_next(lexer_t* l) {
  free(l->current.text);
  if (l->has_peek) {
    l->current = l->peek;
    l->has_peek = false;
    return;
  }
  lexer_fill(l, &l->current);
}

token_t lexer_peek_token(lexer_t* l) {
  if (!l->has_peek) {
    lexer_fill(l, &l->peek);
    l->has_peek = true;
  }
  return l->peek;
}

bool lexer_match(lexer_t* l, token_type_t t) {
  if (l->current.type != t) return false;
  lexer_next(l);
  return true;
}

bool lexer_match_ident(lexer_t* l, const char* s) {
  if (l->current.type != TOK_IDENT) return false;
  if (strcmp(l->current.text, s) != 0) return false;
  lexer_next(l);
  return true;
}

void lexer_expect(lexer_t* l, token_type_t t) {
  if (l->current.type != t)
    die("%s:%d: expected '%s' but got '%s'",
      l->filename, l->current.line, tok_name(t), l->current.text);
  lexer_next(l);
}
