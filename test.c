#include "monoid_crc32.h"
#include <assert.h>
#include <stdio.h>

int main() {
  const char s[] = "123456789";
  monoid_crc32_t c = m_crc32_fold_bytes((void *)s, sizeof(s) - 1);
  assert(c.p == 0x2dfd2d88);
  assert(c.m == 0x1eb014d8);
  assert(m_crc32_finalize(c) == 0xcbf43926);
  const char s1[] = "12345";
  const char s2[] = "6789";
  monoid_crc32_t c1 = m_crc32_fold_bytes((void *)s1, sizeof(s1) - 1);
  monoid_crc32_t c2 = m_crc32_fold_bytes((void *)s2, sizeof(s2) - 1);
  monoid_crc32_t c3 = m_crc32_combine(c1, c2);
  assert(c3.p == c.p);
  assert(c3.m == c.m);
  assert(m_crc32_finalize(c3) == m_crc32_finalize(c));
}
