// SPDX-License-Identifier: GPL-2.0+
/*******************************************************************************
 * This file is the part of the Bytelizer library
 *
 * (C) Copyright 2024 TheSnowfield.
 *
 * Authors: TheSnowfield <17957399+TheSnowfield@users.noreply.github.com>
 ****************************************************************************/

#ifndef _BYTELIZER_ANCHOR_H
#define _BYTELIZER_ANCHOR_H

#include <stdint.h>
#include <string.h>
#include <bytelizer/common.h>

#include "codec.h"
#include "advanced.h"
#include "compiler.h"
#include "debug/log.h"

typedef struct _bytelizer_anchor_t {
  bytelizer_ctx_t old;
  // uint8_t* cursor;
  // uint8_t* block;
  // uint32_t counter;
  // uint32_t total;
  uint32_t userdata;
} bytelizer_anchor_t;

/**
 * @brief get safe anchor with a specific length
 * @param anchor the anchor handle
 * @param ctx the bytelizer context
 * @param size the size to be locked
*/
_inline static bool bytelizer_mark_anchor(bytelizer_anchor_t* anchor,
bytelizer_ctx_t* ctx, uint32_t userdata, uint32_t size) {

  if(bytelizer_ensure_available(ctx, size)) {

    memcpy(&anchor->old, ctx, sizeof(bytelizer_ctx_t));
    anchor->userdata = userdata;

    // // save context information
    // anchor->cursor = ctx->cursor;
    // anchor->total = ctx->total_length;
    // anchor->counter = *ctx->counter;
    // anchor->block = (ctx->blocks != NULL) ? ctx->blocks->tail : ctx->stack;

    // jsut move cursor ahead
    bytelizer_update_cursor(ctx, size);

    return true;
  }

  __bytelizer_log("mark an anchor failed, is it out of memory?");
  return false;
}

/**
 * @brief move the cursor to an anchor
 * @param anchor the anchor handle
 * @param ctx the bytelizer context
 */
_inline static void bytelizer_move_to_anchor(bytelizer_anchor_t* anchor,
bytelizer_ctx_t* ctx) {

  memcpy(ctx, &anchor->old, sizeof(bytelizer_ctx_t));

  // ctx->cursor = anchor->cursor;
  // ctx->blocks->tail = anchor->block;
  // ctx->counter = anchor->counter;

}

#endif /* _BYTELIZER_ANCHOR_H */
