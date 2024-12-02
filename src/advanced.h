// SPDX-License-Identifier: GPL-2.0+
/*******************************************************************************
 * This file is the part of the Bytelizer library
 *
 * (C) Copyright 2024 TheSnowfield.
 *
 * Authors: TheSnowfield <17957399+TheSnowfield@users.noreply.github.com>
 ****************************************************************************/

#ifndef _BYTELIZER_ADVANCED_H
#define _BYTELIZER_ADVANCED_H

#include <stdlib.h>
#include <bytelizer/common.h>
#include <bytelizer/error.h>
#include "codec.h"
#include "bitwise.h"
#include "debug/log.h"

#define MKFLAG(x) (1 << x)

typedef enum {
  prefix_length_only = MKFLAG(0),
  prefix_withself    = MKFLAG(1),
  prefix_uint8       = MKFLAG(2),
  prefix_uint16be    = MKFLAG(3),
  prefix_uint16le    = MKFLAG(4),
  prefix_uint32be    = MKFLAG(5),
  prefix_uint32le    = MKFLAG(6),
  prefix_max         = MKFLAG(7),
} bytelizer_prefix_t;

#undef MKFLAG

/**
 * @brief put bytes into buffer with a length prefix
 * @param ctx the bytelizer context
 * @param value the value
 * @param prefix the length prefix
*/
#define bytelizer_put_bytes_ex(ctx, value, length, prefix) { \
    __generate_prefix((ctx), (prefix), (length)); \
    bytelizer_put_bytes((ctx), (value), (length)); \
  }

/**
 * @brief put another bytelizer into the buffer
 * @param ctx the bytelizer context
 * @param value the bytelizer context to be put
*/
#define bytelizer_put_bytelizer_ex(ctx, value, prefix) { \
  __generate_prefix(ctx, prefix, value->total_length); \
  bytelizer_put_bytelizer(ctx, value); \
}

/**
 * @brief put string into buffer with a length prefix
 * @param ctx the bytelizer context
 * @param value the value
 * @param prefix the length prefix
*/
#define bytelizer_put_string_ex(ctx, value, prefix) \
    bytelizer_put_bytes_ex(ctx, (const uint8_t* const)value, (uint32_t)strlen(value), prefix)

/**
 * @brief put uint16 into the buffer as little endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_put_uint16_le(ctx, value) \
  bytelizer_put_value(ctx, uint16_t, bitwise_le16(value))

/**
 * @brief put uint32 into the buffer as little endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_put_uint32_le(ctx, value) \
  bytelizer_put_value(ctx, uint32_t, bitwise_le32(value))

/**
 * @brief put uint64 into the buffer as little endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_put_uint64_le(ctx, value) \
  bytelizer_put_value(ctx, uint64_t, bitwise_le64(value))

/**
 * @brief put int16 into the buffer as little endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_put_int16_le(ctx, value) \
  bytelizer_put_value(ctx, int16_t, bitwise_le16(value))

/**
 * @brief put int32 into the buffer as little endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_put_int32_le(ctx, value) \
  bytelizer_put_value(ctx, int32_t, bitwise_le32(value))

/**
 * @brief put int64 into the buffer as little endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_put_int64_le(ctx, value) \
  bytelizer_put_value(ctx, int64_t, bitwise_le64(value))

/**
 * @brief put float into the buffer as little endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_put_float_le(ctx, value) \
  bytelizer_put_value(ctx, float, bitwise_le32(value))

/**
 * @brief put double into the buffer as little endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_put_double_le(ctx, value) \
  bytelizer_put_value(ctx, double, bitwise_le64(value))

/**
 * @brief put uint16 into the buffer as big endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_put_uint16_be(ctx, value) \
  bytelizer_put_value(ctx, uint16_t, bitwise_be16(value))

/**
 * @brief put uint32 into the buffer as big endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_put_uint32_be(ctx, value) \
  bytelizer_put_value(ctx, uint32_t, bitwise_be32(value))

/**
 * @brief put uint64 into the buffer as big endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_put_uint64_be(ctx, value) \
  bytelizer_put_value(ctx, uint64_t, bitwise_be64(value))

/**
 * @brief put int16 into the buffer as big endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_put_int16_be(ctx, value) \
  bytelizer_put_value(ctx, int16_t, bitwise_be16(value))

/**
 * @brief put int32 into the buffer as big endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_put_int32_be(ctx, value) \
  bytelizer_put_value(ctx, int32_t, bitwise_be32(value))

/**
 * @brief put int64 into the buffer as big endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_put_int64_be(ctx, value) \
  bytelizer_put_value(ctx, int64_t, bitwise_be64(value))

/**
 * @brief put float into the buffer as big endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_put_float_be(ctx, value) \
  bytelizer_put_value(ctx, float, bitwise_be32(value))

/**
 * @brief put double into the buffer as big endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_put_double_be(ctx, value) \
  bytelizer_put_value(ctx, double, bitwise_be64(value))

/**
 * @brief get uint16 from the buffer as little endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_get_uint16_le(ctx, value) { \
  bytelizer_get_value(ctx, uint16_t, value); \
  *value = bitwise_le16(*value); \
}

/**
 * @brief get uint32 from the buffer as little endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_get_uint32_le(ctx, value) { \
  bytelizer_get_value(ctx, uint32_t, value); \
  *value = bitwise_le32(*value); \
}

/**
 * @brief get uint64 from the buffer as little endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_get_uint64_le(ctx, value) { \
  bytelizer_get_value(ctx, uint64_t, value); \
  *value = bitwise_le64(*value); \
}

/**
 * @brief get int16 from the buffer as little endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_get_int16_le(ctx, value) { \
  bytelizer_get_value(ctx, int16_t, value); \
  *value = bitwise_le16(*value); \
}

/**
 * @brief get int32 from the buffer as little endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_get_int32_le(ctx, value) { \
  bytelizer_get_value(ctx, int32_t, value); \
  *value = bitwise_le32(*value); \
}

/**
 * @brief get int64 from the buffer as little endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_get_int64_le(ctx, value) { \
  bytelizer_get_value(ctx, int64_t, value); \
  *value = bitwise_le64(*value); \
}

/**
 * @brief get float from the buffer as little endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_get_float_le(ctx, value) { \
  bytelizer_get_value(ctx, float, value); \
  *value = bitwise_le32(*value); \
}

/**
 * @brief get double from the buffer as little endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_get_double_le(ctx, value) { \
  bytelizer_get_value(ctx, double, value); \
  *value = bitwise_le64(*value); \
}

/**
 * @brief get uint16 from the buffer as big endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_get_uint16_be(ctx, value) { \
  bytelizer_get_value(ctx, uint16_t, value); \
  *value = bitwise_be16(*value); \
}

/**
 * @brief get uint32 from the buffer as big endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_get_uint32_be(ctx, value) { \
  bytelizer_get_value(ctx, uint32_t, value); \
  *value = bitwise_be32(*value); \
}

/**
 * @brief get uint64 from the buffer as big endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_get_uint64_be(ctx, value) { \
  bytelizer_get_value(ctx, uint64_t, value); \
  *value = bitwise_be64(*value); \
}

/**
 * @brief get int16 from the buffer as big endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_get_int16_be(ctx, value) { \
  bytelizer_get_value(ctx, int16_t, value); \
  *value = bitwise_be16(*value); \
}

/**
 * @brief get int32 from the buffer as big endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_get_int32_be(ctx, value) { \
  bytelizer_get_value(ctx, int32_t, value); \
  *value = bitwise_be32(*value); \
}

/**
 * @brief get int64 from the buffer as big endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_get_int64_be(ctx, value) { \
  bytelizer_get_value(ctx, int64_t, value); \
  *value = bitwise_be64(*value); \
}

/**
 * @brief get float from the buffer as big endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_get_float_be(ctx, value) { \
  bytelizer_get_value(ctx, float, value); \
  *value = bitwise_be32(*value); \
}

/**
 * @brief get double from the buffer as big endian
 * @param ctx the bytelizer context
 * @param value the value
*/
#define bytelizer_get_double_be(ctx, value) { \
  bytelizer_get_value(ctx, double, value); \
  *value = bitwise_be64(*value); \
}

_inline static size_t __get_prefix_length_by_type(bytelizer_prefix_t prefix) {

  switch(prefix) {
    case prefix_uint8: return 1;
    case prefix_uint16be:
    case prefix_uint16le: return 2;
    case prefix_uint32be:
    case prefix_uint32le: return 4;
    default:
      __bytelizer_log("wrong prefix type enum %d", prefix);
      return 0;
  }
}

static void __put_prefix(bytelizer_ctx_t* ctx,
bytelizer_prefix_t base_type, bytelizer_prefix_t length_type, uint32_t value) {

  if(base_type == prefix_withself)
    value += __get_prefix_length_by_type(length_type);

  switch(length_type) {
    case prefix_uint8:
      bytelizer_put_uint8(ctx, (uint8_t)value);
      break;

    case prefix_uint16le:
      bytelizer_put_uint16_le(ctx, (uint16_t)value);
      break;

    case prefix_uint32le:
      bytelizer_put_uint32_le(ctx, value);
      break;

    case prefix_uint16be:
      bytelizer_put_uint16_be(ctx, (uint16_t)value);
      break;

    case prefix_uint32be:
      bytelizer_put_uint32_be(ctx, value);
      break;

    default:
      __bytelizer_log("unsupported prefix type: %d", length_type);
      break;
  }
}

_inline static size_t __get_prefix_length_by_index(bytelizer_prefix_t prefix) {

  static const uint32_t _table_length[] = {
    0, 0, sizeof(uint8_t),
    sizeof(uint16_t), sizeof(uint16_t),
    sizeof(uint32_t), sizeof(uint32_t)
  };

  return _table_length[prefix];
}

static void __parse_prefix(bytelizer_prefix_t prefix, uint32_t srcsize,
bytelizer_prefix_t* base_type, bytelizer_prefix_t* length_type) {

  bytelizer_prefix_t _base_type = prefix_length_only;
  bytelizer_prefix_t _length_type = prefix_uint8;

  // without the prefix length itself
  // in other words, the plain data size
  if(prefix & prefix_length_only) {
    _base_type = prefix_length_only;
  }

  // with prefix length itself
  else if (prefix & prefix_withself) {
    _base_type = prefix_withself;
  }

  else {
        __bytelizer_log("invalid prefix type %d. "
                         "use default prefix 'prefix_length_only' instead", prefix);
  }

  // calculate prefix length
  // ignored prefix_length and prefix_withself
  for(int i = 2, j = prefix_uint8; j < prefix_max; ++i, j <<= 1) {

    // prefix length type
    if(prefix & j) {
      _length_type = (bytelizer_prefix_t)j;
      break;
    }
  }

  if(base_type) *base_type = _base_type;
  if(length_type) *length_type = _length_type;
}

static bool __generate_prefix(bytelizer_ctx_t* ctx,
bytelizer_prefix_t prefix, uint32_t srcsize) {

  bytelizer_prefix_t _basetype, _lentype;

  // get prefix args
  __parse_prefix(prefix, srcsize, &_basetype, &_lentype);

  // put prefix
  __put_prefix(ctx, _basetype, _lentype, srcsize);
  return true;
}

/**
 * @brief put bytes string
 * @param ctx the bytelizer context
 * @param value the bytes value
 * @param length the length of bytes
 * @param prefix the prefix see @ref bytelizer_prefix_t
 */
_inline static void bytelizer_put_bytestr(bytelizer_ctx_t* ctx,
uint8_t* value, uint32_t length, bytelizer_prefix_t prefix) {

  if(length <= 0) {
    __bytelizer_log("wrong size of the value");
    return;
  }

  // calculate result length
  int32_t _hexstr_len = length << 1; {
    if(!__generate_prefix(ctx, prefix, _hexstr_len)) {
      __bytelizer_log("prefix generation failed");
      return;
    }
    
    if(!bytelizer_ensure_available(ctx, _hexstr_len)) {
      __bytelizer_log("out of memory?");
      return;
    }
  }

  int32_t _counter = _hexstr_len;
  static char* _hex_table = "0123456789ABCDEF";

  while(--_counter >= 0) {

    // aligned with 2
    uint8_t _byte = value[_counter >> 1];

    // teardown one byte into high and low parts
    // example: 0xE9 became 0xE and 0x9
    uint8_t _part = _byte >> ((1 - ((_counter & 1))) << 2);

    // match the result from the hex table
    ctx->cursor[_counter] = _hex_table[_part & 0x0F];
  }

  // update the cursor
  bytelizer_update_cursor(ctx, _hexstr_len);
}

#endif /* _BYTELIZER_ADVANCED_H */
