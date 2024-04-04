#include <stdio.h>
#include <bytelizer/codec.h>

int main() {

  // 1. create a bytelizer instance
  // claimed stack allocate 32 bytes for initial use
  bytelizer_alloc(_ctx, 32); {

    // 2. put "hello world" into buffer
    bytelizer_put_string(_ctx, "hello world!");

    // 3. put \0 terminal char
    bytelizer_put_uint8(_ctx, 0x00);
  }

  // 4. check it out
  puts(_ctx->stack);

  // 5. don't forget to destroy the
  // instance after you don't use it anymore
  bytelizer_destroy(_ctx);
}
