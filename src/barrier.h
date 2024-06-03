// SPDX-License-Identifier: GPL-2.0+
/*******************************************************************************
 * This file is the part of the Bytelizer library
 *
 * (C) Copyright 2024 TheSnowfield.
 *
 * Authors: TheSnowfield <17957399+TheSnowfield@users.noreply.github.com>
 ****************************************************************************/

#ifndef _BYTELIZER_BARRIER_H
#define _BYTELIZER_BARRIER_H

#include <bytelizer/common.h>

#include "compiler.h"
#include "codec.h"
#include "advanced.h"
#include "anchor.h"
#include "bitwise.h"
#include "debug/internal.h"

typedef struct {
  bytelizer_ctx_t* ref;
  bytelizer_anchor_t anchor;
  uint32_t prefix_len;
  bytelizer_prefix_t prefix_basetype;
  bytelizer_prefix_t prefix_lentype;
} bytelizer_barrier_t;

_inline static bool __barrier_enter(bytelizer_barrier_t* barrier,
bytelizer_ctx_t* ref, bytelizer_prefix_t prefix) {

  uint32_t _value;
  bytelizer_prefix_t _basetype, _lentype; {
    __parse_prefix(prefix, 0, &_basetype, &_lentype);
    _value = __get_prefix_length_by_type(_lentype);
  }

  if(bytelizer_mark_anchor(&barrier->anchor,
    ref, ref->total_length, _value)) {
    
    barrier->ref = ref;
    barrier->prefix_len = _value;
    barrier->prefix_basetype = _basetype;
    barrier->prefix_lentype = _lentype;

    return true;
  }

  __bytelizer_log("enter barrier failed.");
  return false;
}

static bool __barrier_leave(bytelizer_barrier_t* barrier, uint32_t offset) {

  uint32_t length = 0; {
    length += offset;
    length += barrier->ref->total_length - barrier->anchor.userdata;
    
    // remove self length
    // because (total_length - anchor.userdata) including prefix length
    if(barrier->prefix_basetype == prefix_length_only)
      length -= barrier->prefix_len;
  }

  // write anchor value
  switch(barrier->prefix_lentype) {
    case prefix_uint8:
      bytelizer_write_value_unsafe(barrier->anchor.old.cursor, uint8_t, (uint8_t)length);
      break;

    case prefix_uint16le:
      bytelizer_write_value_unsafe(barrier->anchor.old.cursor, uint16_t, bitwise_le16((uint16_t)length));
      break;

    case prefix_uint32le:
      bytelizer_write_value_unsafe(barrier->anchor.old.cursor, uint32_t, bitwise_le32(length));
      break;

    case prefix_uint16be:
      bytelizer_write_value_unsafe(barrier->anchor.old.cursor, uint16_t, bitwise_be16((uint16_t)length));
      break;

    case prefix_uint32be:
      bytelizer_write_value_unsafe(barrier->anchor.old.cursor, uint32_t, bitwise_be32(length));
      break;

    default:
     __bytelizer_log("unsupported prefix type: %d", barrier->prefix_lentype);
      return false;
  }

  return true;
}

#define bytelizer_barrier_enter(barrier, ref, prefix) \
  bytelizer_barrier_t _barrier##barrier; \
  __barrier_enter(&_barrier##barrier, ref, prefix)

#define bytelizer_barrier_leave(barrier)\
  __barrier_leave(&_barrier##barrier, 0)

#define bytelizer_barrier_leave_offset(barrier, offset)\
  __barrier_leave(&_barrier##barrier, offset)

#endif /* _BYTELIZER_BARRIER_H */
