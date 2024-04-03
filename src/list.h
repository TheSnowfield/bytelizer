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

typedef struct _list_node_t {
  struct _list_node_t* next;
  struct _list_node_t* prev;
  void* data;
} list_node_t;

typedef struct _list_ctx_t {
  list_node_t* head;
  list_node_t* tail;
  size_t size;
} list_ctx_t;

/**
 * @brief create a new list
 *
 * @param ctx return list context if success
 * @return bytelizer_ret_t
 */
bytelizer_ret_t bytelizer_list_create(list_ctx_t** ctx);

/**
 * @brief put data into list
 *
 * @param ctx list context pointer
 * @param data data pointer
 * @param length data length
 * @param node return node pointer
 * @return bytelizer_ret_t
 */
bytelizer_ret_t bytelizer_list_put(list_ctx_t* ctx, void* data, size_t length, list_node_t** node);

bytelizer_ret_t bytelizer_list_delete(list_ctx_t* ctx, list_node_t* node);

/**
 * @brief destroy list
 *
 * @param ctx list context pointer
 * @return bytelizer_ret_t
 */
bytelizer_ret_t bytelizer_list_destroy(list_ctx_t* ctx);


#endif /* _BYTELIZER_LIST_H */
