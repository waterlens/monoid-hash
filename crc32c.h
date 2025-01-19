#include <stddef.h>
#include <stdint.h>

#if defined(_MSC_VER)
#define CRC_AINLINE static __forceinline
#define CRC_ALIGN(n) __declspec(align(n))
#else
#define CRC_AINLINE static __inline __attribute__((always_inline))
#define CRC_ALIGN(n) __attribute__((aligned(n)))
#endif
#define CRC_EXPORT extern

CRC_EXPORT uint32_t crc32c(const char *buf, size_t len);
CRC_EXPORT uint32_t xnmodp(uint64_t n);

CRC_AINLINE uint32_t gf2p32_xtimes(uint32_t a) {
  return (a >> 1) ^ ((0 - (a & 1)) & 0x82F63B78);
}

CRC_AINLINE uint32_t gf2p32_mul(uint32_t a, uint32_t b) {
  uint32_t r = 0;
  while (b) {
    r ^= (0 - (b >> 31)) & a;
    a = gf2p32_xtimes(a);
    b <<= 1;
  }
  return r;
}


typedef struct m_crc32c_s {
  uint32_t p;
  uint32_t m;
} m_crc32c_t;

CRC_AINLINE m_crc32c_t m_crc32c_identity() {
  return (m_crc32c_t){0, 0x80000000};
}

static m_crc32c_t m_crc32c_combine(m_crc32c_t a, m_crc32c_t b) {
  return (m_crc32c_t){gf2p32_mul(a.p, b.m) ^ b.p, gf2p32_mul(a.m, b.m)};
}

CRC_AINLINE m_crc32c_t m_crc32c_fold_bytes(const char *data, size_t len) {
  return (m_crc32c_t){crc32c(data, len), xnmodp(len * 8)};
}

CRC_AINLINE uint32_t m_crc32c_finalize(m_crc32c_t a) {
  return ~(a.p ^ gf2p32_mul(0xffffffff, a.m));
}
