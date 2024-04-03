// SPDX-License-Identifier: GPL-2.0+
/*******************************************************************************
 * This file is the part of the Bytelizer library
 *
 * (C) Copyright 2024 TheSnowfield.
 *
 * Authors: TheSnowfield <17957399+TheSnowfield@users.noreply.github.com>
 ****************************************************************************/

#ifndef _BYTELIZER_LIST_H
#define _BYTELIZER_LIST_H

#include <stddef.h>
#include <bytelizer/error.h>

typedef struct _bytelizer_list_node_t {
  struct _bytelizer_list_node_t* next;
  struct _bytelizer_list_node_t* prev;
  void* data;
} bytelizer_list_node_t;

typedef struct _bytelizer_list_ctx_t {
  bytelizer_list_node_t* head;
  bytelizer_list_node_t* tail;
  size_t size;
} bytelizer_list_ctx_t;

/**
 * @brief create a new list
 *
 * @param ctx return list context if success
 * @return bytelizer_ret_t
 */
bytelizer_ret_t bytelizer_list_create(bytelizer_list_ctx_t** ctx);

/**
 * @brief put data into list
 *
 * @param ctx list context pointer
 * @param data data pointer
 * @param length data length
 * @param node return node pointer
 * @return bytelizer_ret_t
 */
bytelizer_ret_t bytelizer_list_put(bytelizer_list_ctx_t* ctx, void* data, size_t length, bytelizer_list_node_t** node);

/**
 * @brief delete node
 *
 * @param ctx list context pointer
 * @param node node pointer
 * @return bytelizer_ret_t
 */
bytelizer_ret_t bytelizer_list_delete(bytelizer_list_ctx_t* ctx, bytelizer_list_node_t* node);

/**
 * @brief destroy list
 *
 * @param ctx list context pointer
 * @return bytelizer_ret_t
 */
bytelizer_ret_t bytelizer_list_destroy(bytelizer_list_ctx_t* ctx);


#endif /* _BYTELIZER_LIST_H */
