#include <stdio.h>
#include <bytelizer/codec.h>
#include <bytelizer/protobuf.h>

typedef enum {
  example_type_notify = 0,
  example_type_request,
} example_type_t;

typedef enum {
  example_action_buy = 0,
  example_action_sell
} example_action_t;

// declare a protobuf structure
// the name of the structure is _pb_struct_buy_an_apple
// the field name is optional, you can use _ to make a placeholder
PBSTRUCT_EXPORT(buy_an_apple, (PBSTRUCT {

  // example protocol flag
  PB_VARINT (1, flag, example_type_request),  /* root.1     */
  PB_VARINT (2, version, 1),                  /* root.2     */

  // nested protobuf structure
  PB_MESSAGE (3, _, (PBSTRUCT {               /* root.3     */
    PB_VARINT(1, action, example_action_buy), /* root.3.1   */
    PB_MESSAGE (2, _, (PBSTRUCT {             /* root.3.2   */
      PB_VARINT(1, weight, 30),               /* root.3.2.1 */
      PB_VARINT(2, price, 1),                 /* root.3.2.2 */
      PB_FIXED32(3, fee, 0.01),               /* root.3.2.3 */
    PB_MESSAGE_END })),
  PB_MESSAGE_END })),

PB_MESSAGE_END }));

int main() {

  // 1. create a bytelizer instance
  // claimed stack allocate 32 bytes for initial use
  bytelizer_alloc(_ctx, 32); {

    // 2. put protobuf struct into buffer
    bytelizer_put_pbstruct(_ctx, _pb_struct_buy_an_apple);
  }

  // 3. don't forget to destroy the
  // instance after you don't use it anymore
  bytelizer_destroy(_ctx);
}
