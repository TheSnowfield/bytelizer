// SPDX-License-Identifier: GPL-2.0+
/*******************************************************************************
 * This file is the part of the Bytelizer library
 *
 * (C) Copyright 2024 TheSnowfield.
 *
 * Authors: TheSnowfield <17957399+TheSnowfield@users.noreply.github.com>
 ****************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <bytelizer/common.h>
#include <bytelizer/error.h>
#include "debug/internal.h"
#include "list.h"
#include "codec.h"

/*
  Example of how Bytelizer stores uint32_t x 5 = 20 bytes
  with only 10 bytes stack allocation, and BYTELIZER_REALLOC is 8

                         ____ Chained List ____
                        /                      \
  +-----------------+
  | +--> stack      +-> buffer1  --->  buffer2
  | |   [4 bytes]      [4 bytes]       [4 bytes]
  | |   [4 bytes]      [4 bytes]       [pending] <-------+
  | |   [ empty ]                                        |
  | |                                                    |
  | |   The structure of context is                      |
  | |                                                    |
  | |   typedef struct _bytelizer_ctx_t {                |
  | |     uint32_t total_length;                         |
  | +---  uint8_t* stack;                                |
  |       uint32_t stack_wrotes;                         |
  |       uint32_t stack_length;                         |
  +-----  bytelizer_list_ctx_t* blocks;                  |
          uint8_t* cursor;   ----------------------------+
          uint32_t* counter;
        } bytelizer_ctx_t;
*/

static bool __new_block(bytelizer_ctx_t* ctx, uint32_t size) {

  bytelizer_ret_t _result;

  // prepare the node
  uint32_t _block_length = sizeof(bytelizer_block_t) + size;
  bytelizer_block_t* _block = (bytelizer_block_t *)malloc(_block_length); {

    // oops memory allocation failure
    if(_block == NULL) {
      return false;
    }

    // put the new block into list
    if(!bytelizer_ok(_result, bytelizer_list_put(ctx->blocks, &_block, sizeof(void *), NULL))) {
      __bytelizer_log("failure while trying to append buffer block: code %d", _result);
      
      // cleanup
      free(_block);
      return false;
    }

    // initialize the new block
    memset(_block, 0x00, _block_length); {
      _block->wrotes = 0;
      _block->length = size;
    }

    // setup the cursor
    ctx->cursor = (uint8_t *)_block + sizeof(bytelizer_block_t);
    ctx->counter = &_block->wrotes;

    __bytelizer_log("new buffer block [%p], %zu bytes", _block, size);
  }

  return true;
}

#define ALLOC_ALIGNMENT(x) \
  (x < BYTELIZER_REALLOC \
    ? BYTELIZER_REALLOC \
    : (x + (sizeof(size_t) - 1)) & ~(sizeof(size_t) - 1))

/**
 * @brief try expand if the buffer is not enough
 * @param ctx the bytelizer context
 * @param request the request size
*/
bool bytelizer_ensure_available(bytelizer_ctx_t* ctx, size_t request) {

  if(ctx->blocks == NULL) {

    // if the request size is larger than the stack available size
    if(request > (ctx->stack_length - ctx->stack_wrotes)) {

      // create a heap
      bytelizer_ret_t _result;
      if((_result = bytelizer_list_create(&ctx->blocks)) != bytelizer_ret_ok) {
        __bytelizer_log("creating heap buffer failure: code %d", _result);
        return false;
      }

      // cleanup
      if(!__new_block(ctx, ALLOC_ALIGNMENT(request))) {
        bytelizer_list_destroy(ctx->blocks);
        ctx->blocks = NULL;
      }

      return true;
    }
  }

  else {

    // if bytelizer has created a heap buffer before
    // so check it out :p

    bytelizer_block_t* _block = *(bytelizer_block_t **)ctx->blocks->tail->data; {

      // we need a new block
      if(request > (_block->length - _block->wrotes))
        return __new_block(ctx, ALLOC_ALIGNMENT(request));
    }
  }

  return true;
}

static uint32_t bytelizer_peek_available(bytelizer_ctx_t* ctx) {

  if(ctx->blocks == NULL) {
    return ctx->stack_length - ctx->stack_wrotes;
  }

  else {
    bytelizer_block_t* _block = *(bytelizer_block_t **)ctx->blocks->tail->data;
    return _block->length - _block->wrotes;
  }
}

void bytelizer_put_bytes(bytelizer_ctx_t* ctx, const uint8_t* const value, uint32_t length) {

  if(value == NULL || length == 0)
    return;

  uint32_t _remain = length;
  for(uint8_t* i = (uint8_t*)&value[0]; i < value + length;) {
    
    // peek buffer remain space
    uint32_t _available = bytelizer_peek_available(ctx);

    // seems remain space is enough
    if(_available >= _remain) {
      memcpy(ctx->cursor, i, _remain); {
        i += _remain;
        bytelizer_update_cursor(ctx, _remain);
      }
    }

    else {

      // copy the available space
      memcpy(ctx->cursor, i, _available); {
        i += _available;
        bytelizer_update_cursor(ctx, _available);
      }

      _remain -= _available;

      if(!bytelizer_ensure_available(ctx, _remain)) {
        __bytelizer_log("put bytes failed");
        return;
      }
    }
  }

}

void bytelizer_put_bytelizer(bytelizer_ctx_t* ctx, bytelizer_ctx_t* value) {

  // write stack first
  bytelizer_put_bytes(ctx, value->stack, value->stack_wrotes);
  
  // write block buffer
  if(value->blocks != NULL) {
    
    bytelizer_list_node_t* _node = value->blocks->head;
    while(_node != NULL) {

      bytelizer_block_t* _block = *(bytelizer_block_t **)_node->data;
      bytelizer_put_bytes(ctx, (uint8_t *)_block + sizeof(bytelizer_block_t), _block->wrotes);

      _node = _node->next;
    }
  }
}

uint32_t bytelizer_copy_to(void* userdata, bytelizer_ctx_t* ctx, bytelizer_callback_copy_t callback) {

  // write stack first
  callback(userdata, ctx->stack, ctx->stack_wrotes);

  // write block buffer
  if(ctx->blocks != NULL) {
    
    bytelizer_list_node_t* _node = ctx->blocks->head;
    while(_node != NULL) {

      bytelizer_block_t* _block = *(bytelizer_block_t **)_node->data;
      callback(userdata, (uint8_t *)_block + sizeof(bytelizer_block_t), _block->wrotes);

      _node = _node->next;
    }
  }

  return ctx->total_length;
}

void bytelizer_destroy_unsafe(bytelizer_ctx_t* ctx) {

  if(ctx->blocks == NULL) return;

  bytelizer_list_node_t* _node = ctx->blocks->head;
  while(_node != NULL) {
    free(*(bytelizer_block_t **)_node->data);
    _node = _node->next;
  }

  bytelizer_list_destroy(ctx->blocks);
}
