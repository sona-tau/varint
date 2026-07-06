#include "test.h"
#include "varint.h"
#include <stdlib.h>
#include <unistd.h>

void test_to_string() {
  printf("test_to_string: running\n");

  struct {
    uint64_t value;
    const char *expected;
  } cases[] = {{0, "0"},
               {1, "1"},
               {9, "9"},
               {10, "10"},
               {42, "42"},
               {100, "100"},
               {127, "127"},
               {128, "128"},
               {255, "255"},
               {256, "256"},
               {1000, "1000"},
               {12345, "12345"},
               {0xFFFF, "65535"},
               {0x10000, "65536"},
               {0xFFFFFFFF, "4294967295"},
               {0xFFFFFFFFFFFFFFFFULL, "18446744073709551615"}};
  size_t n = sizeof(cases) / sizeof(cases[0]);

  for (size_t i = 0; i < n; i++) {
    vint v = varint_new(cases[i].value);
    char *s = varint_to_string(v);
    ASSERT_NOT_NULL(s);
    printf("%zu: expected %zu got %sXXXXXXXXX\n", strlen(s), cases[i].value, s);
    ASSERT_STR_EQ(s, cases[i].expected);
    free(s);
  }

  printf("test_to_string: passed\n");
}

void test_from_string() {
  printf("test_from_string: running\n");

  struct {
    const char *input;
    uint64_t expected;
  } cases[] = {{"0", 0},
               {"1", 1},
               {"9", 9},
               {"10", 10},
               {"42", 42},
               {"100", 100},
               {"127", 127},
               {"128", 128},
               {"255", 255},
               {"256", 256},
               {"1000", 1000},
               {"12345", 12345},
               {"65535", 0xFFFF},
               {"65536", 0x10000},
               {"4294967295", 0xFFFFFFFF},
               {"18446744073709551615", 0xFFFFFFFFFFFFFFFFULL}};
  size_t n = sizeof(cases) / sizeof(cases[0]);

  for (size_t i = 0; i < n; i++) {
    vint v = varint_from_string(cases[i].input);
    ASSERT_NOT_NULL(v);

    vint expected = varint_new(cases[i].expected);
    ASSERT(varint_eq(v, expected));
  }

  printf("test_from_string: passed\n");
}

void test_string_roundtrip() {
  printf("test_string_roundtrip: running\n");

  uint64_t values[] = {0,      1,       9,          10,
                       42,     100,     127,        128,
                       255,    256,     1000,       12345,
                       0xFFFF, 0x10000, 0xFFFFFFFF, 0xFFFFFFFFFFFFFFFFULL};
  size_t n = sizeof(values) / sizeof(values[0]);

  for (size_t i = 0; i < n; i++) {
    vint original = varint_new(values[i]);
    char *s = varint_to_string(original);
    ASSERT_NOT_NULL(s);

    vint parsed = varint_from_string(s);
    ASSERT_NOT_NULL(parsed);
    ASSERT(varint_eq(original, parsed));

    free(s);
  }

  printf("test_string_roundtrip: passed\n");
}

void test_from_string_invalid() {
  printf("test_from_string_invalid: running\n");

  const char *invalid[] = {"", "abc", "a12", " 42", "-1"};
  size_t n = sizeof(invalid) / sizeof(invalid[0]);

  for (size_t i = 0; i < n; i++) {
    vint v = varint_from_string(invalid[i]);
    ASSERT_EQ(v[0], 0);
  }

  printf("test_from_string_invalid: passed\n");
}

int main(void) {
  test_to_string();
  test_from_string();
  test_string_roundtrip();
  test_from_string_invalid();
  printf("All string I/O tests passed.\n");
  return 0;
}
