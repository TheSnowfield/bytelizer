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
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <bitc/bitc.h>
#include <bitc/const.h>
#include <bitc/lexer.h>
#include <bitc/ast.h>

node_t* node_new(node_kind_t kind) {
  node_t* n = (node_t*)calloc(1, sizeof(node_t));
  if (!n) die("oom");
  n->kind = kind;
  return n;
}

void node_add_child(node_t* parent, node_t* child) {
  child->parent = parent;
  if (parent->last_child) {
    parent->last_child->next_sibling = child;
  } else {
    parent->first_child = child;
  }
  parent->last_child = child;
}

void node_add_attr(node_t* node, const char* attr) {
  node->attr_count++;
  node->attrs = (char**)realloc(node->attrs, node->attr_count * sizeof(char*));
  node->attrs[node->attr_count - 1] = xstrdup(attr);
}

void node_print(node_t* n, int depth) {
  for (int i = 0; i < depth; i++) printf("  ");
  printf("%s", node_kind_name(n->kind));
  if (n->name)     printf(" name='%s'", n->name);
  if (n->value)    printf(" value='%s'", n->value);
  if (n->basetype) printf(" basetype='%s'", n->basetype);
  if (n->param)    printf(" param='%s'", n->param);
  if (n->attr_count > 0) {
    printf(" attrs=[");
    for (int i=0; i<n->attr_count; i++)
      printf("'%s'%s", n->attrs[i], i+1<n->attr_count?",":"");
    printf("]");
  }
  printf("\n");
  for (node_t* c = n->first_child; c; c = c->next_sibling)
    node_print(c, depth + 1);
}

void node_free(node_t* n) {
  if (!n) return;
  free(n->name);
  free(n->value);
  free(n->param);
  free(n->basetype);
  for (int i = 0; i < n->attr_count; i++) free(n->attrs[i]);
  free(n->attrs);
  node_t* c = n->first_child;
  while (c) {
    node_t* next = c->next_sibling;
    node_free(c);
    c = next;
  }
  free(n);
}

/* ================================================================
 *  PARSER
 * ================================================================ */

void parser_init(parser_t* p, lexer_t* lex, const char* bits_dir) {
  memset(p, 0, sizeof(*p));
  p->lex = lex;
  p->bits_dir = xstrdup(bits_dir);
  p->root = node_new(NODE_PROGRAM);
}

/* forward decls */
static node_t* parse_block(parser_t* p);
static node_t* parse_field(parser_t* p);
static node_t* parse_body_item(parser_t* p);
static node_t* parse_field_string_tail(parser_t* p, node_t* field);
static node_t* parse_field_type_body(parser_t* p, node_t* field);
static node_t* parse_field_named(parser_t* p, node_t* field);
static node_t* parse_field_value_anon(parser_t* p, node_t* field);

/* parse include: #include <path> */
static node_t* parse_include(parser_t* p) {
  lexer_t* l = p->lex;
  if (!lexer_match(l, TOK_HASH)) return NULL;
  if (!l->current.text || strcmp(l->current.text, "include") != 0) {
    int start_line = l->current.line;
    while (l->current.type != TOK_EOF && l->current.line == start_line)
      lexer_next(l);
    return NULL;
  }
  lexer_next(l); /* eat 'include' */
  lexer_expect(l, TOK_LT);
  char path[1024]; int i = 0;
  while (l->current.type != TOK_GT && l->current.type != TOK_EOF) {
    const char* s = l->current.text;
    while (*s && i < 1023) path[i++] = *s++;
    lexer_next(l);
  }
  path[i] = '\0';
  lexer_expect(l, TOK_GT);

  node_t* n = node_new(NODE_INCLUDE);
  n->line = l->current.line;
  n->value = xstrdup(path);
  return n;
}

static bool is_all_hex(const char* s) {
  if (!s || !*s) return false;
  for (const char* p = s; *p; p++)
    if (!((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F')))
      return false;
  return true;
}

static node_t* parse_hex_bytes(parser_t* p) {
  lexer_t* l = p->lex;
  if (l->current.type != TOK_IDENT && l->current.type != TOK_NUMBER)
    return NULL;
  /* peek: if followed by LBRACE, likely a type instantiation, not hex */
  token_t pk = lexer_peek_token(l);
  if (pk.type == TOK_LBRACE)
    return NULL;
  const char* h = l->current.text;
  int len = (int)strlen(h);
  if (len != 2 || !is_all_hex(h)) return NULL;

  unsigned v;
  if (sscanf(h, "%x", &v) != 1) return NULL;
  lexer_next(l); /* eat byte */
  if (l->current.type == TOK_SEMICOLON) lexer_next(l);

  char buf[32];
  snprintf(buf, sizeof(buf), "%u", v);
  node_t* f = node_new(NODE_FIELD);
  f->line = l->current.line;
  f->basetype = xstrdup(BIT_UINT8);
  f->value = xstrdup(buf);
  return f;
}

/* parse one body member: [optional attrs]... (bytes { } | field;) */
static node_t* parse_body_item(parser_t* p) {
  lexer_t* l = p->lex;

  int   saved_attr_count = 0;
  char** saved_attrs = NULL;

  /* accumulate consecutive [...] blocks (stop at blank lines) */
  int prev_line = -1;
  while (l->current.type == TOK_LBRACKET) {
    /* detect blank-line separator: if we skipped 2+ lines since last ], stop */
    if (prev_line > 0 && l->current.line > prev_line + 1)
      break;

    lexer_next(l);
    char attr[512];
    while (l->current.type != TOK_RBRACKET && l->current.type != TOK_EOF) {
      int ai = 0;
      while (l->current.type != TOK_RBRACKET && l->current.type != TOK_COMMA && l->current.type != TOK_EOF) {
        const char* s = l->current.text;
        while (*s && ai < 511) attr[ai++] = *s++;
        lexer_next(l);
      }
      attr[ai] = '\0';
      if (ai > 0) {
        saved_attr_count++;
        saved_attrs = (char**)realloc(saved_attrs, saved_attr_count * sizeof(char*));
        saved_attrs[saved_attr_count - 1] = xstrdup(attr);
      }
      if (l->current.type == TOK_COMMA) lexer_next(l);
    }
    /* save line of ] BEFORE consuming it */
    int close_line = l->current.line;
    lexer_expect(l, TOK_RBRACKET);
    prev_line = close_line;
  }

  node_t* result = parse_block(p);
  if (!result) result = parse_hex_bytes(p);
  if (!result && l->current.type == TOK_IDENT && strcmp(l->current.text, "declare") != 0)
    result = parse_field(p);

  if (result && saved_attr_count > 0) {
    if (result->attrs) {
      int total = saved_attr_count + result->attr_count;
      char** merged = (char**)realloc(saved_attrs, total * sizeof(char*));
      for (int i = 0; i < result->attr_count; i++)
        merged[saved_attr_count + i] = result->attrs[i];
      free(result->attrs);
      result->attrs = merged;
      result->attr_count = total;
    } else {
      result->attrs = saved_attrs;
      result->attr_count = saved_attr_count;
    }
  } else if (saved_attr_count > 0) {
    /* standalone attrs: input: → NODE_INCLUDE, rest → NODE_UNKNOWN_ATTR */
    bool has_input = false;
    for (int i = 0; i < saved_attr_count; i++)
      if (strncmp(saved_attrs[i], "input:", 6) == 0) has_input = true;

    node_kind_t kind = has_input ? NODE_INCLUDE : NODE_UNKNOWN_ATTR;
    node_t* n = node_new(kind);
    n->line = l->current.line > 0 ? l->current.line - 1 : 1;
    n->value = xstrdup(saved_attrs[0]);

    /* store additional attrs as child nodes for multi-attr blocks */
    for (int i = 1; i < saved_attr_count; i++) {
      node_t* cn = node_new(NODE_UNKNOWN_ATTR);
      cn->value = xstrdup(saved_attrs[i]);
      cn->line = l->current.line > 0 ? l->current.line - 1 : l->current.line;
      node_add_child(n, cn);
    }

    for (int i = 0; i < saved_attr_count; i++) free(saved_attrs[i]);
    free(saved_attrs);
    return n;
  }
  return result;
}

/* parse bytes block: bytes name(param) { ... } */
static node_t* parse_block(parser_t* p) {
  lexer_t* l = p->lex;

  if (!lexer_match_ident(l, BIT_BYTES)) return NULL;

  node_t* block = node_new(NODE_BLOCK);
  block->line = l->current.line;

  /* optional name — but only if next is '{' (otherwise it's a field like 'bytes value;') */
  if (l->current.type == TOK_IDENT && strcmp(l->current.text, "array") != 0) {
    /* peek: if after name+params comes '{' → block, else → field */
    char* save_text = xstrdup(l->current.text); /* must dup before lexer_next frees current.text */
    lexer_next(l);
    if (l->current.type == TOK_LPAREN) {
      while (l->current.type != TOK_RPAREN && l->current.type != TOK_EOF) lexer_next(l);
      if (l->current.type == TOK_RPAREN) lexer_next(l);
    }
    if (l->current.type != TOK_LBRACE) {
      /* it's a field — undo the name consumption by faking the parse */
      /* We can't undo, but we know this should be handled by parse_field.
         Return NULL and let the caller try parse_field.
         However, we've already consumed BIT_BYTES + name tokens.
         To make this work, we store these as field attrs and return a field node. */
      node_t* field = node_new(NODE_FIELD);
      field->line = block->line;
      field->basetype = xstrdup(BIT_BYTES);
      field->name = save_text; /* transfers ownership */
      node_free(block);
      /* if there are params, skip them (they were consumed above) */
      return field;
    }
    /* it's a block — proceed with name already consumed */
    block->name = save_text; /* transfers ownership */
    /* optional (params) */
    if (l->current.type == TOK_LPAREN) {
      lexer_next(l);
      if (l->current.type == TOK_IDENT) {
        block->param = xstrdup(l->current.text);
        lexer_next(l);
      }
      if (l->current.type == TOK_COMMA) {
        lexer_next(l);
        if (l->current.type != TOK_RPAREN) lexer_next(l);
      }
      lexer_expect(l, TOK_RPAREN);
    }
    lexer_expect(l, TOK_LBRACE);
  } else {
    lexer_expect(l, TOK_LBRACE);
  }

  /* parse body */
  while (l->current.type != TOK_RBRACE && l->current.type != TOK_EOF) {
    node_t* sub = parse_body_item(p);
    if (sub) node_add_child(block, sub);
    else {
      fprintf(stderr, "%s:%d: warning: unexpected token '%s', skipping\n",
        l->filename, l->current.line, l->current.text);
      while (l->current.type != TOK_SEMICOLON &&
             l->current.type != TOK_RBRACE &&
             l->current.type != TOK_EOF)
        lexer_next(l);
      if (l->current.type == TOK_SEMICOLON) lexer_next(l);
    }
  }
  lexer_expect(l, TOK_RBRACE);
  return block;
}

/* ── parse_field helpers ── */

static void parse_type_body(parser_t* p, node_t* field) {
  lexer_t* l = p->lex;
  p->in_type_body++;
  while (l->current.type != TOK_RBRACE && l->current.type != TOK_EOF) {
    node_t* sub = parse_body_item(p);
    if (sub) node_add_child(field, sub);
    else {
      while (l->current.type != TOK_SEMICOLON &&
             l->current.type != TOK_RBRACE &&
             l->current.type != TOK_EOF)
        lexer_next(l);
      if (l->current.type == TOK_SEMICOLON) lexer_next(l);
    }
  }
  p->in_type_body--;
}

static node_t* parse_field_string_tail(parser_t* p, node_t* field) {
  lexer_t* l = p->lex;
  bool has_content = false;
  while (l->current.type != TOK_SEMICOLON && l->current.type != TOK_RBRACE && l->current.type != TOK_EOF) {
    if (l->current.type == TOK_STRING) {
      if (!has_content) { field->value = xstrdup(l->current.text); has_content = true; }
      lexer_next(l);
    } else if (l->current.type == TOK_IDENT) {
      if (field->name) {
        while (l->current.type != TOK_SEMICOLON && l->current.type != TOK_EOF) lexer_next(l);
        break;
      }
      field->name = xstrdup(l->current.text);
      lexer_next(l);
      if (l->current.type == TOK_STRING) {
        while (l->current.type != TOK_SEMICOLON && l->current.type != TOK_EOF) lexer_next(l);
        break;
      }
    } else if (l->current.type == TOK_DOLLAR) {
      lexer_next(l);
      if (l->current.type == TOK_LBRACE) {
        lexer_next(l);
        if (l->current.type == TOK_IDENT) {
          size_t nl = strlen(l->current.text) + 6;
          char* ref = (char*)malloc(nl);
          snprintf(ref, nl, "$ref:%s", l->current.text);
          field->value = ref;
          lexer_next(l);
        }
        if (l->current.type == TOK_RBRACE) lexer_next(l);
      }
    } else {
      lexer_next(l);
    }
  }
  lexer_expect(l, TOK_SEMICOLON);
  return field;
}

static node_t* parse_field_type_body(parser_t* p, node_t* field) {
  lexer_t* l = p->lex;
  lexer_expect(l, TOK_LBRACE);
  parse_type_body(p, field);
  lexer_expect(l, TOK_RBRACE);
  return field;
}

static node_t* parse_field_named(parser_t* p, node_t* field) {
  lexer_t* l = p->lex;
  token_t pk = lexer_peek_token(l);

  if (pk.type == TOK_LBRACE) {
    field->name = xstrdup(l->current.text);
    lexer_next(l);
    lexer_expect(l, TOK_LBRACE);
    parse_type_body(p, field);
    lexer_expect(l, TOK_RBRACE);
    return field;
  }

  if (isdigit(pk.text[0]) || (pk.text[0] == '0' && (pk.text[1] == 'x' || pk.text[1] == 'X'))) {
    field->name = xstrdup(l->current.text);
    lexer_next(l);
    if (l->current.type == TOK_NUMBER) { field->value = xstrdup(l->current.text); lexer_next(l); }
    lexer_expect(l, TOK_SEMICOLON);
    return field;
  }

  if (pk.type == TOK_SEMICOLON) {
    field->name = xstrdup(l->current.text);
    lexer_next(l);
    lexer_expect(l, TOK_SEMICOLON);
    return field;
  }

  if (pk.type == TOK_NUMBER) {
    field->name = xstrdup(l->current.text);
    lexer_next(l);
    field->value = xstrdup(l->current.text);
    lexer_next(l);
    lexer_expect(l, TOK_SEMICOLON);
    return field;
  }

  /* name-only or dotted/arrow path */
  field->name = xstrdup(l->current.text);
  lexer_next(l);
  while (l->current.type == TOK_DOT || l->current.type == TOK_ARROW) {
    bool is_arrow = (l->current.type == TOK_ARROW);
    lexer_next(l);
    if (l->current.type != TOK_IDENT) {
      fprintf(stderr, "%s:%d: expected identifier after '%s'\n",
              l->filename, l->current.line, is_arrow ? "->" : ".");
      break;
    }
    size_t nl = strlen(field->name) + strlen(l->current.text) + 3;
    char* tmp = (char*)malloc(nl);
    snprintf(tmp, nl, "%s%s%s", field->name,
             is_arrow ? "->" : ".", l->current.text);
    free(field->name);
    field->name = tmp;
    lexer_next(l);
  }
  lexer_expect(l, TOK_SEMICOLON);
  return field;
}

static node_t* parse_field_value_anon(parser_t* p, node_t* field) {
  lexer_t* l = p->lex;

  if (l->current.type == TOK_NUMBER) {
    char buf[1024] = {0};
    int bi = 0;
    while (l->current.type == TOK_NUMBER && l->current.type != TOK_SEMICOLON) {
      if (bi > 0) buf[bi++] = ' ';
      const char* s = l->current.text;
      while (*s && bi < 1023) buf[bi++] = *s++;
      lexer_next(l);
    }
    buf[bi] = '\0';
    field->value = xstrdup(buf);
    lexer_expect(l, TOK_SEMICOLON);
    return field;
  }

  if (l->current.type == TOK_STRING) {
    field->value = xstrdup(l->current.text);
    lexer_next(l);
    lexer_expect(l, TOK_SEMICOLON);
    return field;
  }

  if (l->current.type == TOK_DOLLAR) {
    lexer_next(l);
    if (l->current.type == TOK_LBRACE) {
      lexer_next(l);
      char ref[512] = "$ref:";
      int ri = 5;
      while (l->current.type != TOK_RBRACE && l->current.type != TOK_EOF) {
        const char* s = l->current.text;
        while (*s && ri < 510) ref[ri++] = *s++;
        lexer_next(l);
      }
      ref[ri] = '\0';
      field->value = xstrdup(ref);
      if (l->current.type == TOK_RBRACE) lexer_next(l);
    }
    lexer_expect(l, TOK_SEMICOLON);
    return field;
  }

  /* anonymous field: uint32; */
  if (l->current.type == TOK_SEMICOLON) {
    lexer_next(l);
    return field;
  }

  return NULL;
}

/* parse field: type name [value]; | type value; | string ... */
static node_t* parse_field(parser_t* p) {
  lexer_t* l = p->lex;

  node_t* field = node_new(NODE_FIELD);
  field->line = l->current.line;

  if (l->current.type != TOK_IDENT) { node_free(field); return NULL; }

  field->basetype = xstrdup(l->current.text);
  lexer_next(l);

  /* dispatch by basetype / token */
  if (strcmp(field->basetype, BIT_STRING) == 0)
    return parse_field_string_tail(p, field);

  if (l->current.type == TOK_LBRACE)
    return parse_field_type_body(p, field);

  if (l->current.type == TOK_IDENT) {
    node_t* result = parse_field_named(p, field);
    if (result) return result;
  }

  node_t* result = parse_field_value_anon(p, field);
  if (result) return result;

  lexer_next(l);
  node_free(field);
  return NULL;
}

/* parse declare: declare name : base; | declare name : base { fields } */
static node_t* parse_declare(parser_t* p) {
  lexer_t* l = p->lex;
  if (!lexer_match_ident(l, "declare")) return NULL;

  node_t* n = node_new(NODE_DECLARE);
  n->line = l->line;
  n->name = xstrdup(l->current.text);
  lexer_next(l);
  lexer_expect(l, TOK_COLON);
  n->basetype = xstrdup(l->current.text);
  lexer_next(l);

  if (l->current.type == TOK_LBRACE) {
    lexer_next(l);
    while (l->current.type != TOK_RBRACE && l->current.type != TOK_EOF) {
      node_t* sub = parse_body_item(p);
      if (sub) {
        if (sub->kind == NODE_FIELD && sub->basetype && !sub->value) {
          const char* bt = sub->basetype;
          bool is_primitive = (strcmp(bt, BIT_UINT8) == 0 || strcmp(bt, BIT_UINT16) == 0 ||
                               strcmp(bt, BIT_UINT32) == 0 || strcmp(bt, BIT_UINT64) == 0 ||
                               strcmp(bt, BIT_INT8) == 0 || strcmp(bt, BIT_INT16) == 0 ||
                               strcmp(bt, BIT_INT32) == 0 || strcmp(bt, BIT_INT64) == 0 ||
                               strcmp(bt, BIT_STRING) == 0);
          if (is_primitive) {
            sub->value = xstrdup("0");
            sub->is_default = true;
          }
        }
        node_add_child(n, sub);
      } else {
        while (l->current.type != TOK_SEMICOLON &&
               l->current.type != TOK_RBRACE &&
               l->current.type != TOK_EOF)
          lexer_next(l);
        if (l->current.type == TOK_SEMICOLON) lexer_next(l);
      }
    }
    lexer_expect(l, TOK_RBRACE);
  } else {
    lexer_expect(l, TOK_SEMICOLON);
  }
  return n;
}

bool parser_include_file(parser_t* p, const char* rel_path) {
  char full_path[1024];
  snprintf(full_path, sizeof(full_path), "%s/%s", p->bits_dir, rel_path);

  lexer_t inc_lex;
  lexer_open(&inc_lex, full_path);

  parser_t inc_p;
  parser_init(&inc_p, &inc_lex, p->bits_dir);
  inc_p.root = p->root;  /* share root so NODE_DECLARE goes to the same tree */

  bool ok = parser_parse(&inc_p);
  lexer_close(&inc_lex);
  return ok;
}

bool parser_parse(parser_t* p) {
  lexer_t* l = p->lex;
  while (l->current.type != TOK_EOF) {
    if (l->current.type == TOK_HASH) {
      node_t* inc = parse_include(p);
      if (inc) {
        /* immediately parse included file */
        parser_include_file(p, inc->value);
        node_free(inc);
      }
      continue;
    }

    node_t* stmt = parse_declare(p);
    if (!stmt) stmt = parse_body_item(p);

    if (stmt) {
      if (stmt->kind == NODE_DECLARE && stmt->name) {
        for (node_t* c = p->root->first_child; c; c = c->next_sibling)
          if (c->kind == NODE_DECLARE && c->name && strcmp(c->name, stmt->name) == 0)
            die("bitc: error: duplicate declare '%s' (line %d)", stmt->name, stmt->line);
      }
      node_add_child(p->root, stmt);
    } else if (l->current.type != TOK_EOF) {
      fprintf(stderr, "%s:%d: warning: unexpected token '%s', skipping\n",
        l->filename, l->current.line, l->current.text);
      lexer_next(l);
    }
  }
  return p->error_count == 0;
}
