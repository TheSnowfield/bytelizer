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
#include <stdarg.h>
#include <bitc/codegen_plugin.h>
#include <bitc/utils.h>

bool plugin_start(plugin_ctx_t* ctx, const char* plugin_path, const char* source_name) {
  memset(ctx, 0, sizeof(*ctx));
  snprintf(ctx->tmp_out, sizeof(ctx->tmp_out), "%s_bitc_out.tmp", source_name);

  char cmd[4096];
  snprintf(cmd, sizeof(cmd), "cmd /c \"\"%s\" > \"%s\"\"",
           plugin_path, ctx->tmp_out);
  ctx->pipe = popen(cmd, "w");
  if (!ctx->pipe) {
    fprintf(stderr, "bitc: cannot start plugin '%s'\n", plugin_path);
    return false;
  }
  return true;
}

void plugin_feed(plugin_ctx_t* ctx, const char* attr) {
  if (!ctx->pipe || !attr) return;
  fprintf(ctx->pipe, "%s\n", attr);
  ctx->fed_count++;
}

void plugin_stop(plugin_ctx_t* ctx, FILE* out, const char* plugin_path) {
  if (!ctx->pipe) return;
  pclose(ctx->pipe);
  ctx->pipe = NULL;

  if (ctx->fed_count == 0) return;

  size_t plen;
  char* pout = read_file(ctx->tmp_out, &plen);
  if (pout && plen > 0) {
    size_t w = 0;
    for (size_t r = 0; r < plen; r++)
      if (pout[r] != '\r') pout[w++] = pout[r];
    plen = w;
    while (plen > 0 && pout[plen-1] == '\n')
      pout[--plen] = '\0';
    if (plen > 0) {
      fprintf(out, "\n/* ── plugin: %s ── */\n", plugin_path);
      fwrite(pout, 1, plen, out);
      fprintf(out, "\n");
    }
  }
  free(pout);
  remove(ctx->tmp_out);
}