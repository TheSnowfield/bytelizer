// SPDX-License-Identifier: GPL-2.0+
/*******************************************************************************
 * This file is the part of the Bytelizer library
 *
 * (C) Copyright 2024 TheSnowfield.
 *
 * Authors: TheSnowfield <17957399+TheSnowfield@users.noreply.github.com>
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <bytelizer/error.h>
#include "debug/log.h"
#include "list.h"

bytelizer_ret_t bytelizer_list_create(bytelizer_list_ctx_t** ctx) {

  // validate parameters
  if(ctx == NULL)
    return bytelizer_ret_invalid_parameter;

  // initialize context
  bytelizer_list_ctx_t* _ctx = malloc(sizeof(bytelizer_list_ctx_t)); {
    if(_ctx == NULL)
      return bytelizer_ret_out_of_memory;

    // clear buffer
    memset(_ctx, 0, sizeof(bytelizer_list_ctx_t));
    *ctx = _ctx;
  }

  __bytelizer_log("chain list created %p", _ctx);

  return bytelizer_ret_ok;
}

bytelizer_ret_t bytelizer_list_destroy(bytelizer_list_ctx_t* ctx) {
  if(ctx == NULL)
    return bytelizer_ret_invalid_parameter;

  // free all nodes
  bytelizer_list_node_t* _node = ctx->head;
  while(_node != NULL) {
    bytelizer_list_node_t* _next = _node->next;
    free(_node);
    _node = _next;
  }

  // free context
  free(ctx);
  __bytelizer_log("chain list destroyed %p", ctx);

  return bytelizer_ret_ok;
}

bytelizer_ret_t bytelizer_list_put(bytelizer_list_ctx_t* ctx, void* data, size_t length, bytelizer_list_node_t** node) {
  if(ctx == NULL || data == NULL)
    return bytelizer_ret_invalid_parameter;

  if(ctx->size == SIZE_MAX)
    return bytelizer_ret_out_of_memory;

  // allocate an node + userdata length buffer
  // to reduce memory fragmentation
  size_t _length = sizeof(bytelizer_list_node_t) + length;
  bytelizer_list_node_t* _node = malloc(_length); {
    if(_node == NULL)
      return bytelizer_ret_out_of_memory;
  }

  // initialize node and copy data into
  memset(_node, 0, _length); {

    // setup next node pointer
    _node->next = NULL;

    // setup prev node pointer
    if(ctx->head == NULL) _node->prev = NULL;
    else _node->prev = ctx->tail;

    // copy data
    _node->data = (&_node->data) + 1;
    memcpy(_node->data, data, length);
  }

  // append node to list
  if(ctx->head == NULL) {
    ctx->head = _node;
    ctx->tail = _node;
  }
  else {
    ctx->tail->next = _node;
    ctx->tail = _node;
  }

  ++ctx->size;

  // return node pointer
  if(node != NULL) *node = _node;

  return bytelizer_ret_ok;
}

bytelizer_ret_t bytelizer_list_delete(bytelizer_list_ctx_t* ctx, bytelizer_list_node_t* node) {

  if(ctx == NULL || node == NULL)
    return bytelizer_ret_invalid_parameter;

  __bytelizer_log("delete node %p", node);
  __bytelizer_log("node prev %p, node next %p", node->prev, node->next);

  if(node->prev == NULL)
    ctx->head = node->next;
  else
    node->prev->next = node->next;

  if(node->next == NULL)
    ctx->tail = node->prev;
  else
    node->next->prev = node->prev;

  free(node);

  --ctx->size;

  return bytelizer_ret_ok;
}
