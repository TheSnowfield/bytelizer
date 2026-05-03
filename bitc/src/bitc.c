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
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>

#include <bitc/bitc.h>
#include <bitc/lexer.h>
#include <bitc/ast.h>
#include <bitc/compiler.h>
#include <bitc/pass.h>

int main(int argc, char** argv) {
  const char* input  = NULL;
  const char* output = NULL;
  const char* bits_dir = ".";
  const char* plugin_path = NULL;
  bool optimize = false;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-o") == 0 && i+1 < argc) {
      output = argv[++i];
    } else if (strcmp(argv[i], "-I") == 0 && i+1 < argc) {
      bits_dir = argv[++i];
    } else if (strcmp(argv[i], "--plugin") == 0 && i+1 < argc) {
      plugin_path = argv[++i];
    } else if (strcmp(argv[i], "--optimize") == 0) {
      optimize = true;
    } else if (argv[i][0] != '-') {
      input = argv[i];
    }
  }

  if (!input) {
    fprintf(stderr, "Usage: bitc <input.bit> [-o <output.h>] [-I <bits_dir>] [--plugin <path>] [--optimize]\n");
    return 1;
  }

  /* lex */
  lexer_t lex;
  lexer_open(&lex, input);

  /* parse */
  parser_t parser;
  parser_init(&parser, &lex, bits_dir);
  if (!parser_parse(&parser)) {
    fprintf(stderr, "parse failed with %d errors\n", parser.error_count);
    lexer_close(&lex);
    node_free(parser.root);
    free(parser.bits_dir);
    return 1;
  }

  /* optional: print AST */
  if (getenv("BITC_DEBUG")) {
    printf(";; AST dump:\n");
    node_print(parser.root, 0);
  }

  /* extract source name from input path */
  char src_name[256];
  {
    const char* base = strrchr(input, '/');
    if (!base) base = strrchr(input, '\\');
    if (!base) base = input; else base++;
    const char* dot = strrchr(base, '.');
    int n = dot ? (int)(dot - base) : (int)strlen(base);
    if (n > 200) n = 200;
    memcpy(src_name, base, n);
    src_name[n] = '\0';
  }

  /* init compiler context */
  compiler_t cc;
  memset(&cc, 0, sizeof(cc));
  cc.optimize = optimize;
  cc.argc = argc;
  cc.argv = argv;

  /* collect input names from parsed tree */
  for (node_t* n = parser.root->first_child; n; n = n->next_sibling) {
    if (n->kind != NODE_INCLUDE || !n->value) continue;
    if (strncmp(n->value, "input:", 6) != 0) continue;
    const char* v = n->value + 6;
    while (*v == ' ') v++;
    const char* lt = strchr(v, '<');
    const char* gt = lt ? strchr(lt+1, '>') : NULL;
    const char* name_start;
    if (lt && gt) name_start = gt + 1;
    else { const char* sp = strchr(v, ' '); if (!sp) continue; name_start = sp + 1; }
    while (*name_start == ' ' || *name_start == '*') name_start++;
    if (cc.input_count < 16)
      snprintf(cc.input_names[cc.input_count++], 64, "%s", name_start);
  }

  /* Pass 0: resolve attrs */
  pass0_resolve(parser.root, cc.input_names, cc.input_count);
  cc.root = parser.root;
  pass0_validate(&cc);
  if (cc.errors > 0) {
    fprintf(stderr, "bitc: %d error(s) during validation\n", cc.errors);
    node_free(parser.root);
    free(parser.bits_dir);
    lexer_close(&lex);
    return 1;
  }

  /* generate C */
  if (output) {
    cc.out = fopen(output, "w");
    if (!cc.out) die("cannot open output '%s'", output);
    cc.emit = default_emit;
    gen_c(&cc, parser.root, src_name, plugin_path);
    fclose(cc.out);
    printf("bitc wrotes result to %s\n", output);
  } else {
    cc.out = stdout;
    cc.emit = default_emit;
    gen_c(&cc, parser.root, src_name, plugin_path);
  }

  if (cc.errors > 0) {
    fprintf(stderr, "bitc: %d error(s) during code generation\n", cc.errors);
    node_free(parser.root);
    free(parser.bits_dir);
    lexer_close(&lex);
    return 1;
  }

  /* cleanup */
  node_free(parser.root);
  free(parser.bits_dir);
  lexer_close(&lex);

  return 0;
}
