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

#ifndef _BITC_CODEGEN_PLUGIN_H_
#define _BITC_CODEGEN_PLUGIN_H_

#include <stdio.h>
#include <stdbool.h>

typedef struct {
  FILE* pipe;         /* write to plugin stdin */
  char  tmp_out[512]; /* plugin stdout capture path */
  int   fed_count;    /* number of attrs fed */
} plugin_ctx_t;

bool plugin_start(plugin_ctx_t* ctx, const char* plugin_path, const char* source_name);
void plugin_feed(plugin_ctx_t* ctx, const char* attr);
void plugin_stop(plugin_ctx_t* ctx, FILE* out, const char* plugin_path);

#endif /* _BITC_CODEGEN_PLUGIN_H_ */