#include "monoid_crc32.h"
#include <assert.h>

int main() {
  const char s[] = "123456789";
  ek_monoid_crc32_t c = ek_m_crc32_fold_bytes((void *)s, sizeof(s) - 1);
  assert(c.p == 771566984);
  assert(c.m == 514856152);
  assert(ek_m_crc32_finalize(c) == 3421780262);
}
