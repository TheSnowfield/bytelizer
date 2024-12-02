// SPDX-License-Identifier: GPL-2.0+
/*******************************************************************************
 * This file is the part of the Bytelizer library
 *
 * (C) Copyright 2024 TheSnowfield.
 *
 * Authors: TheSnowfield <17957399+TheSnowfield@users.noreply.github.com>
 ****************************************************************************/

#include <stdint.h>
#include <string.h>

#include "codec.h"
#include "protobuf.h"
#include "advanced.h"
#include "debug/log.h"

_inline static uint32_t __number_tovarint(uint64_t value, uint8_t* varint) {

  uint32_t _len = 0;

  // encode every 8 bits
  if(value >= 127) {
    do{
      varint[_len] = (uint8_t)((value & 0x7f) | 0x80);
      value >>= 7;
      ++_len;
    } while(value > 127);
  }

  varint[_len] = (uint8_t)value;
  ++_len;

  return _len;
}

_inline static uint64_t __varint_to_number(uint8_t* varint, uint8_t varint_len) {
  uint64_t _result = 0;

  for(size_t i = varint_len - 1; i >= 0 ; --i) {
    _result <<= 7;
    _result |= varint[i] & 0x7f;
  }

  return _result;
}

_inline static void bytelizer_put_varint(bytelizer_ctx_t* ctx, uint64_t value) {
  uint8_t _buffer[10];
  uint32_t _length = __number_tovarint(value, _buffer);
  bytelizer_put_bytes(ctx, _buffer, _length);
}

_inline static void bytelizer_get_varint(bytelizer_ctx_t* ctx, uint64_t* value) {
  
  uint8_t _b = 0;
  uint64_t _value = 0;

  do {

    // get one byte
    bytelizer_get_uint8(ctx, &_b);

    // teardown varint bytes
    _value <<= 7;
    _value |= _b & 0x7f;

  } while((_b & 0b10000000) > 0);

  if(value) *value = _value;
}

void bytelizer_put_pbstruct(bytelizer_ctx_t* ctx, const bytelizer_pbfield_t* pbroot) {

  if(pbroot == NULL) return;

  while(pbroot->tag != 0) {

    // put the tag
    bytelizer_put_varint(ctx, pbroot->tag << 3 | pbroot->type);

    // put the value
    switch(pbroot->type) {

      case bytelizer_pbtype_varint:
        bytelizer_put_varint(ctx, pbroot->value.varint);
        break;

      case bytelizer_pbtype_32bit:
        bytelizer_put_uint32(ctx, pbroot->value.fixed32);
        break;
      
      case bytelizer_pbtype_64bit:
        bytelizer_put_uint64(ctx, pbroot->value.fixed64);
        break;

      case bytelizer_pbtype_length_delimited: {

        // allocate new buffer for sub struct encoding
        bytelizer_alloc(_ctx, 512); {
          if(pbroot->subtags)
            bytelizer_put_pbstruct(_ctx, pbroot->value.message);
          else
            bytelizer_put_bytes(_ctx, pbroot->value.length_delimited.data,
                                      pbroot->value.length_delimited.length);
        }

        // put the length in varint
        bytelizer_put_varint(ctx, bytelizer_length(_ctx));

        // copy data
        bytelizer_put_bytelizer(ctx, _ctx);
        bytelizer_destroy(_ctx);
        break;
      }

      default:
        __bytelizer_log("uknown tag %d", pbroot->tag);
    }

    ++pbroot;
  }

}

bool bytelizer_get_pbstruct(bytelizer_ctx_t* ctx, bytelizer_pbfield_t* pbroot) {

  if(pbroot == NULL) return false;
  
  while(pbroot->tag != 0) {

    // get the tag
    uint64_t _varint = 0;
    bytelizer_get_varint(ctx, &_varint);
    bytelizer_pbtype_t _type = (bytelizer_pbtype_t)(_varint & 0b111);
    uint32_t _tag = ((uint32_t)_varint) >> 3;

    // strict type check
    if(pbroot->tag != _tag || pbroot->type != _type) {
      return false;
    }

    // set the value
    switch(pbroot->type) {

      case bytelizer_pbtype_varint: {
        uint64_t _value = 0;
        bytelizer_get_varint(ctx, &_value);
        pbroot->value.varint = _value;
        break;
      }

      case bytelizer_pbtype_32bit: {
        uint32_t _value = 0;
        bytelizer_get_uint32(ctx, &_value);
        pbroot->value.fixed32 = _value;
        break;
      }
      
      case bytelizer_pbtype_64bit: {
        uint64_t _value = 0;
        bytelizer_get_uint64(ctx, &_value);
        pbroot->value.fixed64= _value;
      }

      case bytelizer_pbtype_length_delimited: {
        
        bytelizer_get_varint(ctx, &_varint);

        // pure bytes
        if(!pbroot->subtags) {
          // assume the buffer is linear
          pbroot->value.length_delimited.data = ctx->cursor;
          pbroot->value.length_delimited.length = (uint32_t)_varint;
          bytelizer_update_cursor(ctx, (uint32_t)_varint);
          break;
        }

        // if contains sub tags
        // try decode the struct recursively
        bytelizer_get_pbstruct(ctx, pbroot->value.message);
        break;
      }

      default: 
        break;
    }

    ++pbroot;
  }

  return true;
}
