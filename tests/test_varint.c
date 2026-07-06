#include "test.h"
#include "varint.h"
#include <assert.h>
#include <stdio.h>
#define TOP_MASK 0x80
#define BOT_MASK 0x7F

void test_new() {
  printf("test_new: running\n");
  vint a1 = varint_new(0);
  ASSERT_NOT_NULL(a1);

  vint a2 = varint_new(0x01);
  ASSERT_NOT_NULL(a2);
  ASSERT_EQ(varint_length(a2), 1);
  ASSERT_EQ(a2[0], 0x01);

  vint a3 = varint_new(0x7F);
  ASSERT_NOT_NULL(a3);
  ASSERT_EQ(varint_length(a3), 1);
  ASSERT_EQ(a3[0], 0x7F);

  vint a4 = varint_new(0x80);
  ASSERT_NOT_NULL(a4);
  ASSERT_EQ(varint_length(a4), 2);
  ASSERT_EQ(a4[0], 0x80);
  ASSERT_EQ(a4[1], 0x01);

  vint a5 = varint_new(0xFF);
  ASSERT_NOT_NULL(a5);
  ASSERT_EQ(varint_length(a5), 2);
  ASSERT_EQ(a5[0], 0xFF);
  ASSERT_EQ(a5[1], 0x01);

  vint a6 = varint_new(0x012C);
  ASSERT_NOT_NULL(a6);
  ASSERT_EQ(varint_length(a6), 2);
  ASSERT_EQ(a6[0], 0xAC);
  ASSERT_EQ(a6[1], 0x02);

  vint a7 = varint_new(0x4000);
  ASSERT_NOT_NULL(a7);
  ASSERT_EQ(varint_length(a7), 3);
  ASSERT_EQ(a7[0], 0x80);
  ASSERT_EQ(a7[1], 0x80);
  ASSERT_EQ(a7[2], 0x01);

  vint a8 = varint_new(12345);
  ASSERT_NOT_NULL(a8);
  ASSERT_EQ(varint_length(a8), 2);
  ASSERT_EQ(a8[0], 0xB9);
  ASSERT_EQ(a8[1], 0x60);
  printf("test_new: passed\n");
}

void test_single_byte() {
  printf("test_single_byte: running\n");
  vint v0 = varint_new(0);
  vint v1 = varint_new(1);
  vint v2 = varint_new(2);
  vint v3 = varint_new(3);
  vint v42 = varint_new(42);
  vint v10 = varint_new(10);
  vint v52 = varint_new(52);
  vint v63 = varint_new(63);
  vint v64 = varint_new(64);
  vint v127 = varint_new(127);

  vint r1 = varint_add(.l = v0, .r = v0);
  ASSERT(varint_eq(r1, v0));

  vint r2 = varint_add(.l = v1, .r = v2);
  ASSERT(varint_eq(r2, v3));

  vint r3 = varint_add(.l = v42, .r = v10);
  ASSERT(varint_eq(r3, v52));

  vint r4 = varint_add(.l = v63, .r = v64);
  ASSERT(varint_eq(r4, v127));
  printf("test_single_byte: passed\n");
}

void test_first_carry(void) {
  printf("test_first_carry: running\n");

  vint v1 = varint_new(1);
  vint v2 = varint_new(2);
  vint v126 = varint_new(126);
  vint v127 = varint_new(127);
  vint v128 = varint_new(128);
  vint v254 = varint_new(254);

  vint r1 = varint_add(.l = v127, .r = v1);
  ASSERT(varint_eq(r1, v128));

  vint r2 = varint_add(.l = v127, .r = v127);
  ASSERT(varint_eq(r2, v254));

  vint r3 = varint_add(.l = v126, .r = v2);
  ASSERT(varint_eq(r3, v128));

  printf("test_first_carry: passed\n");
}

void test_different_lengths(void) {
  printf("test_different_lengths: running\n");

  vint v1 = varint_new(1);
  vint v27 = varint_new(27);
  vint v127 = varint_new(127);
  vint v128 = varint_new(128);
  vint v129 = varint_new(129);
  vint v255 = varint_new(255);
  vint v300 = varint_new(300);
  vint v327 = varint_new(327);
  vint v16384 = varint_new(16384);
  vint v16385 = varint_new(16385);

  vint r1 = varint_add(.l = v128, .r = v1);
  ASSERT(varint_eq(r1, v129));

  vint r2 = varint_add(.l = v128, .r = v127);
  ASSERT(varint_eq(r2, v255));

  vint r3 = varint_add(.l = v300, .r = v27);
  ASSERT(varint_eq(r3, v327));

  vint r4 = varint_add(.l = v16384, .r = v1);
  ASSERT(varint_eq(r4, v16385));

  printf("test_different_lengths: passed\n");
}

void test_carry_propagation(void) {
  printf("test_carry_propagation: running\n");

  vint v1 = varint_new(1);
  vint v2 = varint_new(2);
  vint v16383 = varint_new(16383);
  vint v16384 = varint_new(16384);
  vint v16385 = varint_new(16385);
  vint v32768 = varint_new(32768);

  vint r1 = varint_add(.l = v16383, .r = v1);
  ASSERT(varint_eq(r1, v16384));

  vint r2 = varint_add(.l = v16383, .r = v2);
  ASSERT(varint_eq(r2, v16385));

  vint r3 = varint_add(.l = v16384, .r = v16384);
  ASSERT(varint_eq(r3, v32768));

  printf("test_carry_propagation: passed\n");
}

void test_max_uint64(void) {
  printf("test_max_uint64: running\n");

  vint vmax = varint_new(UINT64_MAX);
  vint v0 = varint_new(0);

  vint r = varint_add(.l = vmax, .r = v0);
  ASSERT(varint_eq(r, vmax));

  printf("test_max_uint64: passed\n");
}

void test_uint64_overflow(void) {
  printf("test_uint64_overflow: running\n");

  vint vmax = varint_new(UINT64_MAX);
  vint v1 = varint_new(1);

  varint r = varint_add(.l = vmax, .r = v1);

  ASSERT_EQ(varint_length(r), 10);

  ASSERT_EQ(r[0], 0x80);
  ASSERT_EQ(r[1], 0x80);
  ASSERT_EQ(r[2], 0x80);
  ASSERT_EQ(r[3], 0x80);
  ASSERT_EQ(r[4], 0x80);
  ASSERT_EQ(r[5], 0x80);
  ASSERT_EQ(r[6], 0x80);
  ASSERT_EQ(r[7], 0x80);
  ASSERT_EQ(r[8], 0x80);
  ASSERT_EQ(r[9], 0x02);

  printf("test_uint64_overflow: passed\n");
  free(r);
}

int main(void) {
  printf("\n");

  test_new();
  test_single_byte();
  test_first_carry();
  test_different_lengths();
  test_carry_propagation();
  test_max_uint64();
  test_uint64_overflow();

  printf("test_varint: all tests passed\n");
  return 0;
}
