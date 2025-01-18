#include "monoid_crc32.h"
#include <assert.h>

int main() {
  const char s[] = "123456789";
  ek_monoid_crc32_t c = ek_m_crc32_fold_bytes((void *)s, sizeof(s) - 1);
  assert(c.p == 771566984);
  assert(c.m == 514856152);
  assert(ek_m_crc32_finalize(c) == 3421780262);
  const char s1[] = "12345";
  const char s2[] = "6789";
  ek_monoid_crc32_t c1 = ek_m_crc32_fold_bytes((void *)s1, sizeof(s1) - 1);
  ek_monoid_crc32_t c2 = ek_m_crc32_fold_bytes((void *)s2, sizeof(s2) - 1);
  ek_monoid_crc32_t c3 = ek_m_crc32_combine(c1, c2);
  assert(c3.p == c.p);
  assert(c3.m == c.m);
  assert(ek_m_crc32_finalize(c3) == ek_m_crc32_finalize(c));
}
