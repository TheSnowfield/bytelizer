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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "compiler.h"

_inline uint16_t _swap_16(uint16_t value) {
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

// START https://github.com/fumiama/fumidb/blob/main/include/binary.h
#ifndef __null
#  define __null 0
#endif

#ifdef __linux__
#  include <endian.h>
#endif
#ifdef __FreeBSD__
#  include <sys/endian.h>
#endif
#ifdef __NetBSD__
#  include <sys/endian.h>
#endif
#ifdef __OpenBSD__
#  include <sys/types.h>
#  define be16toh(x) betoh16(x)
#  define be32toh(x) betoh32(x)
#  define be64toh(x) betoh64(x)
#endif
#ifdef __MAC_10_0
#  define be16toh(x) ntohs(x)
#  define be32toh(x) ntohl(x)
#  define be64toh(x) ntohll(x)
#  define htobe16(x) ntohs(x)
#  define htobe32(x) htonl(x)
#  define htobe64(x) htonll(x)
#endif
#ifdef _MSC_VER
  #if BYTELIZER_ENDIANNESS == BYTELIZER_BIG_ENDIAN
    #  define be16toh(x) (x)
    #  define be32toh(x) (x)
    #  define be64toh(x) (x)
    #  define htobe16(x) (x)
    #  define htobe32(x) (x)
    #  define htobe64(x) (x)
  #else
    #  define be16toh(x) _byteswap_ushort(x)
    #  define be32toh(x) _byteswap_ulong(x)
    #  define be64toh(x) _byteswap_uint64(x)
    #  define htobe16(x) _byteswap_ushort(x)
    #  define htobe32(x) _byteswap_ulong(x)
    #  define htobe64(x) _byteswap_uint64(x)
  #endif
#endif
// END https://github.com/fumiama/fumidb/blob/main/include/binary.h

#ifndef be16toh
  #if BYTELIZER_ENDIANNESS == BYTELIZER_BIG_ENDIAN
    #  define be16toh(x) (x)
    #  define htobe16(x) (x)
  #else
    #  define be16toh(x) _swap_16(x)
    #  define htobe16(x) _swap_16(x)
  #endif
#endif
#ifndef be32toh
  #if BYTELIZER_ENDIANNESS == BYTELIZER_BIG_ENDIAN
    #  define be32toh(x) (x)
    #  define htobe32(x) (x)
  #else
    #  define be32toh(x) _swap_32(x)
    #  define htobe32(x) _swap_32(x)
  #endif
#endif
#ifndef be64toh
  #if BYTELIZER_ENDIANNESS == BYTELIZER_BIG_ENDIAN
    #  define be64toh(x) (x)
    #  define htobe64(x) (x)
  #else
    #  define be64toh(x) _swap_64(x)
    #  define htobe64(x) _swap_64(x)
  #endif
#endif

#if BYTELIZER_ENDIANNESS == BYTELIZER_LITTLE_ENDIAN

  #define bitwise_le16(x) (x)
  #define bitwise_le32(x) (x)
  #define bitwise_le64(x) (x)

  #define bitwise_be16(x) be16toh(x)
  #define bitwise_be32(x) be32toh(x)
  #define bitwise_be64(x) be64toh(x)

#else

  #define bitwise_be16(x) (x)
  #define bitwise_be32(x) (x)
  #define bitwise_be64(x) (x)

  #define bitwise_le16(x) _swap_16(x)
  #define bitwise_le32(x) _swap_32(x)
  #define bitwise_le64(x) _swap_64(x)

#endif /* BYTELIZER_ENDIANNESS */

#endif /* _UTILS_BITWISE_H */
