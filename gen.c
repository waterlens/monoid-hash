#include "monoid_crc32.h"
#include <stdio.h>

int main() {
  printf("const gf2p32_t ek_gf2p32_table[256] = {\n");
  for (int i = 0; i < 256; i++) {
    if (i % 8 == 0) {
      printf("  ");
    }
    ek_monoid_crc32_t c = ek_m_crc32_byte(i);
    printf("0x");
    gf2p32_print_hex(c.p);
    printf(", ");
    if (i % 8 == 7) {
      printf("\n");
    }
  }
  printf("};\n");
}