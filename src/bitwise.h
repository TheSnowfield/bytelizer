// SPDX-License-Identifier: GPL-2.0+
/*******************************************************************************
 * This file is the part of the Bytelizer library
 *
 * (C) Copyright 2024 TheSnowfield.
 *
 * Authors: TheSnowfield <17957399+TheSnowfield@users.noreply.github.com>
 ****************************************************************************/

#ifndef _UTILS_BITWISE_H
#define _UTILS_BITWISE_H

#include <stdint.h>
#include "compiler.h"

_inline uint32_t _swap_16(uint16_t value) {
  #ifdef _MSC_VER
    return _byteswap_ushort(value);
  #elif __GNUC__
    return __builtin_bswap16(value);
  #else
    return ((value & 0xff00) >> 8) |
         ((value & 0x00ff) << 8);
  #endif
}

_inline uint32_t _swap_32(uint32_t value) {
  #ifdef _MSC_VER
    return _byteswap_ulong(value);
  #elif __GNUC__
    return __builtin_bswap32(value);
  #else
    return ((value & 0xff000000) >> 24) |
         ((value & 0x00ff0000) >> 8)  |
         ((value & 0x0000ff00) << 8)  |
         ((value & 0x000000ff) << 24);
  #endif
}

_inline uint64_t _swap_64(uint64_t value) {
  #ifdef _MSC_VER
    return _byteswap_uint64(value);
  #elif __GNUC__
    return __builtin_bswap64(value);
  #else
    return ((value & 0xff00000000000000) >> 56) |
       ((value & 0x00ff000000000000) >> 40) |
       ((value & 0x0000ff0000000000) >> 24) |
       ((value & 0x000000ff00000000) >> 8)  |
       ((value & 0x00000000ff000000) << 8)  |
       ((value & 0x0000000000ff0000) << 24) |
       ((value & 0x000000000000ff00) << 40) |
       ((value & 0x00000000000000ff) << 56);
  #endif
}

#if BYTELIZER_ENDIANNESS == BYTELIZER_LITTLE_ENDIAN

  #define bitwise_le16(x) (x)
  #define bitwise_le32(x) (x)
  #define bitwise_le64(x) (x)

  #define bitwise_be16(x) _swap_16(x)
  #define bitwise_be32(x) _swap_32(x)
  #define bitwise_be64(x) _swap_64(x)

#else

  #define bitwise_be16(x) (x)
  #define bitwise_be32(x) (x)
  #define bitwise_be64(x) (x)

  #define bitwise_le16(x) _swap_16(x)
  #define bitwise_le32(x) _swap_32(x)
  #define bitwise_le64(x) _swap_64(x)

#endif /* BYTELIZER_ENDIANNESS */

#endif /* _UTILS_BITWISE_H */
