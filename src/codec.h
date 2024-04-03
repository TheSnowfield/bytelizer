// SPDX-License-Identifier: GPL-2.0+
/*******************************************************************************
 * This file is the part of the Bytelizer library
 *
 * (C) Copyright 2024 TheSnowfield.
 *
 * Authors: TheSnowfield <17957399+TheSnowfield@users.noreply.github.com>
 ****************************************************************************/

#ifndef _BYTELIZER_CODEC_H
#define _BYTELIZER_CODEC_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "list.h"

typedef struct _bytelizer_block_t {
  uint32_t length;
  uint32_t wrotes;
  // the next is a block of memory
  // it's minimal length has been defined in BYTELIZER_REALLOC
} bytelizer_block_t;

typedef struct _bytelizer_ctx_t {
  uint32_t total_length;
  uint8_t* stack;
  uint32_t stack_wrotes;
  uint32_t stack_length;
  bytelizer_list_ctx_t* blocks;
  uint8_t* cursor;
  uint32_t* counter;
} bytelizer_ctx_t;

typedef void (* bytelizer_callback_copy_t)(void* userdata, uint8_t* buffer, size_t length);

/**
 * @brief bytelizer initialize without force clear
 * @param ctx the bytelizer context
 * @param size the stack buffer size
 */
#define bytelizer_alloc_unsafe(ctx, size) \
  uint8_t ctx##_buf[size]; \
  bytelizer_ctx_t* ctx = &(bytelizer_ctx_t) { \
    .stack = ctx##_buf, \
    .stack_length = size, \
    .stack_wrotes = 0, \
    .total_length = 0, \
    .blocks = NULL, \
    .cursor = ctx##_buf, \
  }; { ctx->counter = &ctx->stack_wrotes; memset(ctx##_buf, 0, size); }

/**
 * @brief bytelizer initialize
 * @param ctx the bytelizer context
 * @param size the stack buffer size
 */
#define bytelizer_alloc(ctx, size) { bytelizer_alloc_unsafe(ctx, size)

/**
 * @brief release heap blocks and clean without pairing
 * @param ctx the bytelizer context
 */
#define bytelizer_clear_unsafe(ctx) \
    bytelizer_destroy(ctx); \
    ctx->stack_wrotes = 0; \
    ctx->total_length = 0; \
    ctx->blocks = NULL; \
    ctx->cursor = ctx->stack; \
    ctx->counter = &ctx->stack_wrotes; \
    memset(ctx->stack, 0, ctx->stack_length); \

/**
 * @brief release heap blocks and clean
 * @param ctx the bytelizer context
 */
#define bytelizer_clear(ctx) bytelizer_clear_unsafe(ctx) }

/**
 * @brief attach a buffer without force detach
 * @param ctx the bytelizer context
 * @param buffer the buffer pointer
 * @param size the buffer size
 */
#define bytelizer_attach_unsafe(ctx, buffer, size) \
  bytelizer_ctx_t* ctx = &(bytelizer_ctx_t) { \
    .stack = buffer, \
    .stack_length = size, \
    .stack_wrotes = size, \
    .total_length = size, \
    .blocks = NULL, \
    .cursor = buffer, \
  }; { ctx->counter = &ctx->stack_wrotes; }

/**
 * @brief attach a buffer
 * @param ctx the bytelizer context
 * @param buffer the buffer pointer
 * @param size the buffer size
 */
#define bytelizer_attach(ctx, buffer, size) { bytelizer_attach_unsafe(ctx, buffer, size)

/**
 * @brief release heap blocks and clean without pairing
 * @param ctx the bytelizer context
 */
#define bytelizer_detach_unsafe(ctx) \
    bytelizer_destroy(ctx); \
    ctx->stack_wrotes = 0; \
    ctx->total_length = 0; \
    ctx->blocks = NULL; \
    ctx->cursor = ctx->stack; \
    ctx->counter = &ctx->stack_wrotes; \
    memset(ctx->stack, 0, ctx->stack_length); \

/**
 * @brief release heap blocks and clean
 * @param ctx the bytelizer context
 */
#define bytelizer_detach(ctx) bytelizer_detach_unsafe(ctx) }

/**
 * @brief get bytelizer length
 * @param ctx the bytelizer context
 */
#define bytelizer_length(ctx) (ctx->total_length)

#define bytelizer_update_cursor(ctx, size) { \
  ctx->cursor += size; \
  ctx->total_length += size; \
  *ctx->counter += size; \
}

/**
 * @brief put value into buffer (unsafe)
 * @param ctx the bytelizer context
 * @param value the value to put
*/
#define bytelizer_put_value_unsafe(ctx, type, value) \
  *(type *)ctx->cursor = value; \
  bytelizer_update_cursor(ctx, sizeof(type));

/**
 * @brief put value into buffer
 * @param ctx the bytelizer context
 * @param value the value to put
*/
#define bytelizer_put_value(ctx, type, value) \
  if(bytelizer_ensure_available(ctx, sizeof(type))) { \
    *(type *)ctx->cursor = value; \
    bytelizer_update_cursor(ctx, sizeof(type)); \
  }

#define bytelizer_get_value(ctx, type, value) \
  *(value) = *(type *)ctx->cursor; \
  bytelizer_update_cursor(ctx, sizeof(type));

/**
 * @brief write(without increase length) value into buffer (unsafe)
 * @param ctx the bytelizer context
 * @param value the value to put
*/
#define bytelizer_write_value_unsafe(cursor, type, value) \
  *(type *)cursor = value;

/**
 * @brief read(without decrease length) value from buffer (unsafe)
 * @param ctx the bytelizer context
 * @param value the value to read
*/
#define bytelizer_read_value_unsafe(cursor, type, value) \
  value = *(type *)cursor;

/**
 * @brief put uint8
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_put_uint8(ctx, value) \
  bytelizer_put_value(ctx, uint8_t, value)

/**
 * @brief get uint8
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_get_uint8(ctx, value) \
  bytelizer_get_value(ctx, uint8_t, value)

/**
 * @brief put uint16 into the buffer as platform endianness
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_put_uint16(ctx, value) \
  bytelizer_put_value(ctx, uint16_t, value)

/**
 * @brief get uint16 into the buffer as platform endianness
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_get_uint16(ctx, value) \
  bytelizer_get_value(ctx, uint16_t, value)

/**
 * @brief put uint32 into the buffer as platform endianness
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_put_uint32(ctx, value) \
  bytelizer_put_value(ctx, uint32_t, value)

/**
 * @brief get uint32 into the buffer as platform endianness
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_get_uint32(ctx, value) \
  bytelizer_get_value(ctx, uint32_t, value)

/**
 * @brief put uint64 into the buffer as platform endianness
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_put_uint64(ctx, value) \
  bytelizer_put_value(ctx, uint64_t, value)

/**
 * @brief get uint64 into the buffer as platform endianness
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_get_uint64(ctx, value) \
  bytelizer_get_value(ctx, uint64_t, value)

/**
 * @brief put int8 into the buffer as platform endianness
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_put_int8(ctx, value) \
  bytelizer_put_value(ctx, int8_t, value)

/**
 * @brief put int16 into the buffer as platform endianness
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_put_int16(ctx, value) \
  bytelizer_put_value(ctx, int16_t, value)

/**
 * @brief put int32 into the buffer as platform endianness
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_put_int32(ctx, value) \
  bytelizer_put_value(ctx, int32_t, value)

/**
 * @brief put int64 into the buffer as platform endianness
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_put_int64(ctx, value) \
  bytelizer_put_value(ctx, int64_t, value)

/**
 * @brief put string into buffer
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_put_string(ctx, value) \
  bytelizer_put_bytes(ctx, (uint8_t *)value, (uint32_t)strlen(value));

#define bytelizer_skip bytelizer_update_cursor

/**
 * @brief put bytes into buffer
 * @param ctx the bytelizer context
 * @param value the value
 * @param length the length of value
*/
void bytelizer_put_bytes(bytelizer_ctx_t* ctx, const uint8_t* const value, uint32_t length);

/**
 * @brief put another bytelizer buffer into buffer
*/
void bytelizer_put_bytelizer(bytelizer_ctx_t* ctx, bytelizer_ctx_t* value);

/**
 * @brief ensure the buffer is available for writing
 * @param ctx the bytelizer context
 * @param request the request length
*/
bool bytelizer_ensure_available(bytelizer_ctx_t* ctx, size_t request);

/**
 * @brief copy buffer to callback
 * @param userdat user data
 * @param ctx the bytelizer context
 * @param callback the callback function
 * @return the total length of wrote
*/
uint32_t bytelizer_copy_to(void* userdata, bytelizer_ctx_t* ctx, bytelizer_callback_copy_t callback);

/**
 * @brief dectroy bytelizer
 * @param ctx the bytelizer context
*/
void bytelizer_destroy(bytelizer_ctx_t* ctx);

#endif /* _BYTELIZER_CODEC_H */
