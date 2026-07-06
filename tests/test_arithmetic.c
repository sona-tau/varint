#include "test.h"
#include "varint.h"

void test_add() {
  printf("test_add: running\n");

  vint v0 = varint_new(0);
  vint v1 = varint_new(1);
  vint v2 = varint_new(2);
  vint v3 = varint_new(3);
  vint v10 = varint_new(10);
  vint v42 = varint_new(42);
  vint v52 = varint_new(52);
  vint v63 = varint_new(63);
  vint v64 = varint_new(64);
  vint v127 = varint_new(127);
  vint v128 = varint_new(128);

  /* identity and zero */
  vint r1 = varint_add(.l = v0, .r = v0);
  ASSERT(varint_eq(r1, v0));

  vint r2 = varint_add(.l = v1, .r = v0);
  ASSERT(varint_eq(r2, v1));

  /* single-byte, no carry */
  vint r3 = varint_add(.l = v1, .r = v2);
  ASSERT(varint_eq(r3, v3));

  vint r4 = varint_add(.l = v42, .r = v10);
  ASSERT(varint_eq(r4, v52));

  /* single-byte boundary: 63 + 64 = 127 */
  vint r5 = varint_add(.l = v63, .r = v64);
  ASSERT(varint_eq(r5, v127));

  /* carry into two bytes: 127 + 1 = 128 */
  vint r6 = varint_add(.l = v127, .r = v1);
  ASSERT(varint_eq(r6, v128));

  /* commutativity: 42 + 10 == 10 + 42 */
  vint r7 = varint_add(.l = v10, .r = v42);
  ASSERT(varint_eq(r7, r4));

  printf("test_add: passed\n");
}

void test_sub() {
  printf("test_sub: running\n");

  vint v0 = varint_new(0);
  vint v1 = varint_new(1);
  vint v2 = varint_new(2);
  vint v3 = varint_new(3);
  vint v10 = varint_new(10);
  vint v42 = varint_new(42);
  vint v52 = varint_new(52);
  vint v63 = varint_new(63);
  vint v64 = varint_new(64);
  vint v127 = varint_new(127);
  vint v128 = varint_new(128);

  /* same value */
  vint r1 = varint_sub(.l = v0, .r = v0);
  ASSERT(varint_eq(r1, v0));

  vint r2 = varint_sub(.l = v1, .r = v1);
  ASSERT(varint_eq(r2, v0));

  /* simple single-byte */
  vint r3 = varint_sub(.l = v3, .r = v2);
  ASSERT(varint_eq(r3, v1));

  vint r4 = varint_sub(.l = v52, .r = v10);
  ASSERT(varint_eq(r4, v42));

  /* boundary: 127 - 64 = 63 */
  vint r5 = varint_sub(.l = v127, .r = v64);
  ASSERT(varint_eq(r5, v63));

  /* multi-byte borrow: 128 - 1 = 127 */
  vint r6 = varint_sub(.l = v128, .r = v1);
  ASSERT(varint_eq(r6, v127));

  /* multi-byte borrow: 128 - 64 = 64 */
  vint r7 = varint_sub(.l = v128, .r = v64);
  ASSERT(varint_eq(r7, v64));

  printf("test_sub: passed\n");
}

void test_mul() {
  printf("test_mul: running\n");

  vint v0 = varint_new(0);
  vint v1 = varint_new(1);
  vint v2 = varint_new(2);
  vint v3 = varint_new(3);
  vint v5 = varint_new(5);
  vint v6 = varint_new(6);
  vint v10 = varint_new(10);
  vint v42 = varint_new(42);
  vint v100 = varint_new(100);
  vint v420 = varint_new(420);

  /* identity and zero */
  vint r1 = varint_mul(.l = v0, .r = v42);
  ASSERT(varint_eq(r1, v0));

  vint r2 = varint_mul(.l = v1, .r = v42);
  ASSERT(varint_eq(r2, v42));

  /* single-byte */
  vint r3 = varint_mul(.l = v2, .r = v3);
  ASSERT(varint_eq(r3, v6));

  /* carry into multi-byte: 10 * 10 = 100 */
  vint r4 = varint_mul(.l = v10, .r = v10);
  ASSERT(varint_eq(r4, v100));

  /* multi-byte: 42 * 10 = 420 */
  vint r5 = varint_mul(.l = v42, .r = v10);
  ASSERT(varint_eq(r5, v420));

  /* commutativity: 10 * 42 == 42 * 10 */
  vint r6 = varint_mul(.l = v10, .r = v42);
  ASSERT(varint_eq(r6, r5));

  /* 5 * 5 = 25, then 25 * 5 = 125 */
  vint r7 = varint_mul(.l = v5, .r = v5);
  vint expected125 = varint_new(25);
  ASSERT(varint_eq(r7, expected125));

  printf("test_mul: passed\n");
}

void test_div10() {
  printf("test_div10: running\n");

  vint v0 = varint_new(0);
  vint v1 = varint_new(1);
  vint v9 = varint_new(9);
  vint v10 = varint_new(10);
  vint v42 = varint_new(42);
  vint v100 = varint_new(100);
  vint v420 = varint_new(420);

  /* 0 / 10 = 0 rem 0 */
  divmod10 d1 = varint_div10(v0);
  ASSERT(varint_eq(d1.q, v0));
  ASSERT_EQ(d1.r, 0);

  /* 1 / 10 = 0 rem 1 */
  divmod10 d2 = varint_div10(v1);
  ASSERT(varint_eq(d2.q, v0));
  ASSERT_EQ(d2.r, 1);

  /* 9 / 10 = 0 rem 9 */
  divmod10 d3 = varint_div10(v9);
  ASSERT(varint_eq(d3.q, v0));
  ASSERT_EQ(d3.r, 9);

  /* 10 / 10 = 1 rem 0 */
  divmod10 d4 = varint_div10(v10);
  ASSERT(varint_eq(d4.q, v1));
  ASSERT_EQ(d4.r, 0);

  /* 42 / 10 = 4 rem 2 */
  divmod10 d5 = varint_div10(v42);
  vint v4 = varint_new(4);
  ASSERT(varint_eq(d5.q, v4));
  ASSERT_EQ(d5.r, 2);

  /* 100 / 10 = 10 rem 0 */
  divmod10 d6 = varint_div10(v100);
  ASSERT(varint_eq(d6.q, v10));
  ASSERT_EQ(d6.r, 0);

  /* 420 / 10 = 42 rem 0 */
  divmod10 d7 = varint_div10(v420);
  ASSERT(varint_eq(d7.q, v42));
  ASSERT_EQ(d7.r, 0);

  free(d1.q);
  free(d2.q);
  free(d3.q);
  free(d4.q);
  free(d5.q);
  free(d6.q);
  free(d7.q);

  printf("test_div10: passed\n");
}

int main(void) {
  test_add();
  test_sub();
  test_mul();
  test_div10();
  printf("All tests passed.\n");
  return 0;
}
