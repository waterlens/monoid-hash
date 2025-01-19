#include "crc32c.h"
#include <assert.h>
#include <stdio.h>

int main() {
  uint32_t r = crc32c("123456789", 9);
  assert(r == 0x58E3FA20);
  m_crc32c_t a = m_crc32c_fold_bytes("123456789", 9);
  assert(a.p == 0x58E3FA20);
  assert(m_crc32c_finalize(a) == 0xE3069283);
  m_crc32c_t split1 = m_crc32c_fold_bytes("12345", 5);
  m_crc32c_t split2 = m_crc32c_fold_bytes("6789", 4);
  m_crc32c_t combined = m_crc32c_combine(split1, split2);
  assert(m_crc32c_finalize(combined) == 0xE3069283);
}