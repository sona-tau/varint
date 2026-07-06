#include "varint.h"
#include <assert.h>
#include <errno.h>
#include <memory.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define TOP_BIT (uint8_t)0x80
#define BOT_MASK ~TOP_BIT
#define TOP_MASK ~BOT_MASK

static inline void *_oom_check(void *p, const char *file, size_t line,
                               const char *func) {
  if (p == NULL) {
    fprintf(stderr, "%s:%zu: in %s: Could not allocate memory; KAB-OOM: %s\n",
            file, line, func, strerror(errno));
    exit(EXIT_FAILURE);
  }
  return p;
}

#define oom_check(p) _oom_check(p, __FILE__, __LINE__, __func__)

varint varint_new(uint64_t a) {
  uint8_t tmp[10];
  size_t len = 0;

  do {
    tmp[len++] = (uint8_t)(a & 0x7F);
    a >>= 7;
  } while (a);

  varint ret = oom_check(calloc(1, len));

  for (size_t i = 0; i < len; ++i)
    ret[i] = tmp[i] | TOP_BIT;

  ret[len - 1] &= (uint8_t)BOT_MASK;
  return ret;
}

size_t varint_into(varint *out, size_t nbytes, uint64_t a) {
  assert(out != NULL);
  uint8_t tmp[10];
  size_t len = 0;

  do {
    tmp[len++] = (uint8_t)(a & 0x7F);
    a >>= 7;
  } while (a);

  if (len > nbytes) {
    fprintf(stderr,
            "%s:%d: error: varint does not fit in provided space, expected %zu "
            "<= %zu\n",
            __FILE__, __LINE__, len, nbytes);
    return 0;
  } else {
    for (size_t i = 0; i < len; ++i)
      (*out)[i] = tmp[i] | (uint8_t)TOP_BIT;

    (*out)[len - 1] &= (uint8_t)BOT_MASK;
    return len;
  }
}

varint _varint_add(varint a, size_t lena, varint b, size_t lenb,
                   Deallocate deallocate) {
  assert(varint_length(a) == lena &&
         "`lenl` must be exactly equal to the length of `l`");
  assert(varint_length(b) == lenb &&
         "`lenr` must be exactly equal to the length of `r`");

  uint8_t carry = 0;
  size_t n = lena > lenb ? lena : lenb;
  size_t len = 0;
  varint ret = oom_check(calloc(1, n + 1));

  for (size_t i = 0; i < n; ++i) {
    uint8_t acc = carry;

    if (i < lena)
      acc += a[i] & (uint8_t)BOT_MASK;

    if (i < lenb)
      acc += b[i] & (uint8_t)BOT_MASK;

    ret[i] = acc & (uint8_t)BOT_MASK;
    carry = acc >> 7;
    len += 1;
  }

  if (carry) {
    ret[len++] = carry;
  } else {
    varint tmp = oom_check(realloc(ret, len));
    ret = tmp;
  }

  for (size_t i = 0; i < len - 1; ++i)
    ret[i] |= TOP_BIT;
  ret[len - 1] &= (uint8_t)BOT_MASK;

  switch (deallocate) {
  case Both:
    free(a);
    free(b);
    break;
  case Left:
    free(a);
    break;
  case Right:
    free(b);
    break;
  case Neither:
    break;
  default:
    break;
  }

  return ret;
}

varint _varint_sub(varint a, size_t na, varint b, size_t nb,
                   Deallocate deallocate) {
  assert(varint_length(a) == na &&
         "`lenl` must be exactly equal to the length of `l`");
  assert(varint_length(b) == nb &&
         "`lenr` must be exactly equal to the length of `r`");
  assert((varint_gt(a, b) || varint_eq(a, b)) &&
         "`l >= r` in order for subtraction to be defined");

  size_t n = na;

  varint r = oom_check(calloc(n, 1));

  int borrow = 0;

  for (size_t i = 0; i < n; i++) {
    int ai = a[i] & 0x7F;
    int bi = (i < nb) ? (b[i] & 0x7F) : 0;

    int diff = ai - bi - borrow;

    if (diff < 0) {
      diff += 128;
      borrow = 1;
    } else {
      borrow = 0;
    }

    r[i] = diff & 0x7F;
  }

  while (n > 1 && (r[n - 1] & 0x7F) == 0)
    n--;

  for (size_t i = 0; i + 1 < n; i++)
    r[i] |= 0x80;

  r[n - 1] &= 0x7F;

  switch (deallocate) {
  case Both:
    free(a);
    free(b);
    break;
  case Left:
    free(a);
    break;
  case Right:
    free(b);
    break;
  case Neither:
    break;
  default:
    break;
  }

  return r;
}

varint _varint_mul(varint a, size_t na, varint b, size_t nb,
                   Deallocate deallocate) {
  assert(varint_length(a) == na &&
         "`lenl` must be exactly equal to the length of `l`");
  assert(varint_length(b) == nb &&
         "`lenr` must be exactly equal to the length of `r`");
  size_t n = na + nb;
  uint32_t *tmp = oom_check(calloc(n, sizeof(uint16_t)));

  for (size_t i = 0; i < na; i++)
    for (size_t j = 0; j < nb; j++)
      tmp[i + j] +=
          (uint32_t)((a[i] & (uint8_t)BOT_MASK) * (b[j] & (uint8_t)BOT_MASK));

  for (size_t i = 0; i < n; i++) {
    uint32_t carry = tmp[i] >> 7;
    tmp[i] &= 0x7F;

    if (i + 1 < n)
      tmp[i + 1] += carry;
  }

  while (n > 1 && tmp[n - 1] == 0)
    n--;

  varint r = oom_check(calloc(n, 1));

  for (size_t i = 0; i < n; i++)
    r[i] = tmp[i] & 0x7F;

  free(tmp);

  for (size_t i = 0; i + 1 < n; i++)
    r[i] |= 0x80;

  r[n - 1] &= 0x7F;

  switch (deallocate) {
  case Both:
    free(a);
    free(b);
    break;
  case Left:
    free(a);
    break;
  case Right:
    free(b);
    break;
  case Neither:
    break;
  default:
    break;
  }

  return r;
}

void varint_print_debug(varint a) {
  assert(a != NULL);
  size_t i = 0;
  printf("0x");
  i = 0;
  for (;;) {
    printf("%02x", (uint8_t)(a[i]));
    if ((a[i] & TOP_MASK) == 0)
      break;
    ++i;
  }
}

char *varint_to_string(varint a) {
  if (a[0] == 0) {
    char *str = oom_check(malloc(2));
    str[0] = '0';
    str[1] = '\0';
    return str;
  }

  size_t n = varint_length(a);
  varint last_q = oom_check(calloc(1, n));
  memcpy(last_q, a, n);

  char *str = oom_check(calloc(1, n * 3 + 1));
  oom_check(str);

  size_t len = 0;
  do {
    divmod10 dm = varint_div10(last_q);
    free(last_q);
    last_q = dm.q;
    str[len++] = (char)dm.r + '0';
    char new_char = (char)dm.r + '0';
    printf("%c", new_char);
  } while (last_q[0] != 0);
  free(last_q);

  // reverse string
  for (size_t i = 0; i < len / 2; ++i) {
    char tmp = str[i];
    str[i] = str[len - i - 1];
    str[len - i - 1] = tmp;
  }
  str[len] = '\0';

  return str;
}

varint varint_from_string(const char *s) {
  varint ret = varint_new(0);
  size_t retn = varint_length(ret);

  uint8_t v10buf[1] = {0};
  varint v10 = v10buf;
  size_t v10n = varint_into(&v10, 1, 10);

  uint8_t vvalbuf[1] = {0};
  varint vval = vvalbuf;
  size_t vvaln = varint_into(&vval, 1, 0);

  size_t sn = strlen(s);

  for (size_t i = 0; i < sn; ++i) {
    char c = s[i];
    if (c >= '0' && c <= '9') {
      uint8_t value = (uint8_t)c - '0';
      vvaln = varint_into(&vval, 1, (uint64_t)value);

      ret = varint_add(.l = varint_mul(.l = ret, .lenl = retn, .r = v10,
                                       .lenr = v10n, .deallocate = Left),
                       .r = vval, .lenr = vvaln, .deallocate = Left);
      retn = varint_length(ret);
    } else {
      break;
    }
  }

  return ret;
}

size_t varint_length(varint a) {
  size_t acc = 1;
  for (size_t i = 0; (a[i] & TOP_MASK) != 0; ++i)
    acc += 1;
  return acc;
}

divmod10 varint_div10(varint a) {
  size_t n = varint_length(a);

  varint q = oom_check(calloc(n, 1));
  uint16_t rem = 0;

  for (size_t i = n; i-- > 0;) {
    uint16_t cur = (uint16_t)(rem * 128 + (a[i] & BOT_MASK));

    q[i] = (uint8_t)(cur / 10);
    rem = cur % 10;
  }

  size_t q_len = n;
  while (q_len > 1 && q[q_len - 1] == 0)
    q_len--;

  varint q_trim = oom_check(malloc(q_len));
  memcpy(q_trim, q, q_len);
  free(q);

  return (divmod10){.q = q_trim, .r = (uint8_t)rem};
}

int varint_cmp(varint a, varint b) {
  size_t lena = varint_length(a);
  size_t lenb = varint_length(b);

  if (lena > lenb)
    return 1;

  if (lena < lenb)
    return -1;

  for (size_t i = 0; i < lenb; ++i) {
    uint8_t l = a[i] & (uint8_t)BOT_MASK;
    uint8_t r = b[i] & (uint8_t)BOT_MASK;
    if (l < r)
      return -1;
    if (l > r)
      return 1;
  }
  return 0;
}
