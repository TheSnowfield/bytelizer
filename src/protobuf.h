// SPDX-License-Identifier: GPL-2.0+
/*******************************************************************************
 * This file is the part of the Bytelizer library
 *
 * (C) Copyright 2024 TheSnowfield.
 *
 * Authors: TheSnowfield <17957399+TheSnowfield@users.noreply.github.com>
 ****************************************************************************/

#ifndef _BYTELIZER_PROTOBUF_H
#define _BYTELIZER_PROTOBUF_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

typedef enum {
  bytelizer_pbtype_varint = 0,
  bytelizer_pbtype_64bit = 1,
  bytelizer_pbtype_length_delimited = 2,
  bytelizer_pbtype_32bit = 5,
} bytelizer_pbtype_t;

typedef struct _bytelizer_pfield_t {
  uint32_t tag;
  bytelizer_pbtype_t type;
  bool subtags;
  union {
    uint64_t varint;
    uint64_t fixed64;
    struct {
      uint32_t length;
      uint8_t* data;
    } length_delimited;
    uint32_t fixed32;
    struct _bytelizer_pfield_t* message;
  } value;
} bytelizer_pbfield_t;

void bytelizer_put_pbstruct(bytelizer_ctx_t* ctx, const bytelizer_pbfield_t* pbroot);

/**
 * @brief define a varint field
 * @param _tag the field index
 * @param _name the name of the field
 * @param _val the value of the field
*/
#define PB_VARINT(_tag, _name, _val) \
  {.tag = _tag, .type = bytelizer_pbtype_varint, .value.varint = _val }

/**
 * @brief define a fix32 field
 * @param _tag the field index
 * @param _name the name of the field
 * @param _val the value of the field
*/
#define PB_FIXED32(_tag, _name, _val) \
  {.tag = _tag, .type = bytelizer_pbtype_32bit, .value.fixed32 = _val }

/**
 * @brief define a fix64 field
 * @param _tag the field index
 * @param _name the name of the field
 * @param _val the value of the field
*/
#define PB_FIXED64(_tag, _name, _val) \
  {.tag = _tag, .type = bytelizer_pbtype_64bit, .value.fixed64 = _val }

/**
 * @brief define a float field
 * @param _tag the field index
 * @param _name the name of the field
 * @param _val the value of the field
*/
#define PB_FLOAT(_tag, _name, _val) \
  PB_FIXED32(_tag, _name, (*(uint32_t *)_val))

/**
 * @brief define a double field
 * @param _tag the field index
 * @param _name the name of the field
 * @param _val the value of the field
*/
#define PB_DOUBLE(_tag, _name, _val) \
  PB_FIXED64(_tag, _name, (*(uint64_t *)_val))

/**
 * @brief define a string field
 * @param _tag the field index
 * @param _name the name of the field
 * @param _val the value of the field
*/
#define PB_CSTRING(_tag, _name, _val) \
  {.tag = _tag, .type = bytelizer_pbtype_length_delimited, .subtags = false, .value.length_delimited = { .length = sizeof(_val) - 1, .data = (uint8_t*)_val }}

/**
 * @brief define a string field
 * @param _tag the field index
 * @param _name the name of the field
 * @param _val the value of the field
 * @param _len the length of the field
*/
#define PB_STRING(_tag, _name, _val, _len) \
  {.tag = _tag, .type = bytelizer_pbtype_length_delimited, .subtags = false, .value.length_delimited = { .length = _len, .data = (uint8_t*)_val }}

/**
 * @brief define a bytes field
 * @param _tag the field index
 * @param _name the name of the field
 * @param _val the value of the field
 * @param _len the length of the field
*/
#define PB_BYTES(_tag, _name, _val, _len) \
  {.tag = _tag, .type = bytelizer_pbtype_length_delimited, .subtags = false, .value.length_delimited = { .length = _len, .data = (uint8_t*)_val }}

/**
 * @brief define a protobuf message struct
 * @param _tag the field index
 * @param _name the name of the field
 * @param _val the value of the field
*/
#define PB_MESSAGE(_tag, _name, _val) \
  {.tag = _tag, .type = bytelizer_pbtype_length_delimited, .subtags = true, .value.message = _val }

#define PB_MESSAGE_END {.tag = 0 }

/**
 * @brief protobuf struct start
*/
#define PBSTRUCT (bytelizer_pbfield_t [])

/**
 * @brief export a protobuf struct
*/
#define PBSTRUCT_EXPORT(_name, _fields) \
  const static bytelizer_pbfield_t* _pb_struct_##_name = _fields

#endif /* _BYTELIZER_PROTOBUF_H */
