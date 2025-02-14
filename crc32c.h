#ifndef CRC32C_H
#define CRC32C_H

#include <stddef.h>
#include <stdint.h>

#if defined(_MSC_VER)
#define CRC_AINLINE static __forceinline
#define CRC_ALIGN(n) __declspec(align(n))
#else
#define CRC_AINLINE static __inline __attribute__((always_inline))
#define CRC_ALIGN(n) __attribute__((aligned(n)))
#endif
#ifndef CRC_EXPORT
#define CRC_EXPORT static
#endif

CRC_EXPORT uint32_t crc32c(const char *buf, size_t len);
CRC_EXPORT uint32_t xnmodp(uint64_t n);
CRC_EXPORT uint32_t clmulr32(uint32_t a, uint32_t b);

typedef struct m_crc32c_s {
  uint32_t p;
  uint32_t m;
} m_crc32c_t;

CRC_AINLINE m_crc32c_t m_crc32c_identity() {
  return (m_crc32c_t){0, 0x80000000};
}

static m_crc32c_t m_crc32c_combine(m_crc32c_t a, m_crc32c_t b) {
  return (m_crc32c_t){clmulr32(a.p, b.m) ^ b.p, clmulr32(a.m, b.m)};
}

CRC_AINLINE m_crc32c_t m_crc32c_fold_bytes(const char *data, size_t len) {
  return (m_crc32c_t){crc32c(data, len), xnmodp(len * 8)};
}

CRC_AINLINE uint32_t m_crc32c_finalize(m_crc32c_t a) {
  return ~(a.p ^ clmulr32(0xffffffff, a.m));
}

#if defined(__aarch64__) && (__ARM_ARCH == 8) && defined(__ARM_FEATURE_CRC32)

/* Generated by https://github.com/corsix/fast-crc32/ using: */
/* ./generate -i neon -p crc32c -a v12 */
/* MIT licensed */

#include <arm_acle.h>
#include <arm_neon.h>
#include <stdint.h>

CRC_AINLINE uint64x2_t clmul_lo(uint64x2_t a, uint64x2_t b) {
  uint64x2_t r;
  __asm("pmull %0.1q, %1.1d, %2.1d\n" : "=w"(r) : "w"(a), "w"(b));
  return r;
}

CRC_AINLINE uint64x2_t clmul_lo_e(uint64x2_t a, uint64x2_t b, uint64x2_t c) {
  uint64x2_t r;
  __asm("pmull %0.1q, %2.1d, %3.1d\neor %0.16b, %0.16b, %1.16b\n"
        : "=w"(r), "+w"(c)
        : "w"(a), "w"(b));
  return r;
}

CRC_AINLINE uint64x2_t clmul_hi_e(uint64x2_t a, uint64x2_t b, uint64x2_t c) {
  uint64x2_t r;
  __asm("pmull2 %0.1q, %2.2d, %3.2d\neor %0.16b, %0.16b, %1.16b\n"
        : "=w"(r), "+w"(c)
        : "w"(a), "w"(b));
  return r;
}

CRC_EXPORT
uint32_t clmulr32(uint32_t a, uint32_t b) {
  uint64x2_t aa = vdupq_n_u64(a), bb = vdupq_n_u64(b);
  uint64x2_t j = vshlq_u64(clmul_lo(aa, bb), vdupq_n_s64(1));
  uint64x2_t k;
  {
    static const uint64_t CRC_ALIGN(16) k_[] = {
        0x90d3d871bd4e27e2ull,
    };
    k = vld1q_dup_u64(k_);
  }
  uint64x2_t l = clmul_lo(j, k);
  {
    static const uint64_t CRC_ALIGN(16)
        k_[] = {0xfffffffe00000000ull, (uint64_t)-1ll};
    k = vld1q_u64(k_);
  }
  uint64x2_t n = vandq_u64(l, k);
  {
    static const uint64_t CRC_ALIGN(16) k_[] = {0x82f63b7880000000ull};
    k = vld1q_dup_u64(k_);
  }
  uint64x2_t hi = clmul_lo(n, k);
  uint64x2_t shl = vextq_u64(hi, vdupq_n_u64(0), 1);
  uint64x2_t r = clmul_hi_e(n, k, shl);
  return (uint32_t)vgetq_lane_u64(r, 0);
}

CRC_EXPORT uint32_t xnmodp(uint64_t n) /* x^n mod P, in log(n) time */ {
  uint64_t stack = ~(uint64_t)1;
  uint32_t acc, low;
  for (; n > 191; n = (n >> 1) - 16) {
    stack = (stack << 1) + (n & 1);
  }
  stack = ~stack;
  acc = ((uint32_t)0x80000000) >> (n & 31);
  for (n >>= 5; n; --n) {
    acc = __crc32cw(acc, 0);
  }
  while ((low = stack & 1), stack >>= 1) {
    uint64x2_t x =
        vreinterpretq_u64_s32(vsetq_lane_s32(acc, vdupq_n_s32(0), 0));
    uint64_t y =
        vgetq_lane_u64(vreinterpretq_u64_p128(vmull_p64(vgetq_lane_u64(x, 0),
                                                        vgetq_lane_u64(x, 0))),
                       0);
    acc = __crc32cd(0, y << low);
  }
  return acc;
}

CRC_EXPORT uint32_t crc32c(const char *buf, size_t len) {
  uint32_t crc0 = 0;
  for (; len && ((uintptr_t)buf & 7); --len) {
    crc0 = __crc32cb(crc0, *buf++);
  }
  if (((uintptr_t)buf & 8) && len >= 8) {
    crc0 = __crc32cd(crc0, *(const uint64_t *)buf);
    buf += 8;
    len -= 8;
  }
  if (len >= 192) {
    /* First vector chunk. */
    uint64x2_t x0 = vld1q_u64((const uint64_t *)buf), y0;
    uint64x2_t x1 = vld1q_u64((const uint64_t *)(buf + 16)), y1;
    uint64x2_t x2 = vld1q_u64((const uint64_t *)(buf + 32)), y2;
    uint64x2_t x3 = vld1q_u64((const uint64_t *)(buf + 48)), y3;
    uint64x2_t x4 = vld1q_u64((const uint64_t *)(buf + 64)), y4;
    uint64x2_t x5 = vld1q_u64((const uint64_t *)(buf + 80)), y5;
    uint64x2_t x6 = vld1q_u64((const uint64_t *)(buf + 96)), y6;
    uint64x2_t x7 = vld1q_u64((const uint64_t *)(buf + 112)), y7;
    uint64x2_t x8 = vld1q_u64((const uint64_t *)(buf + 128)), y8;
    uint64x2_t x9 = vld1q_u64((const uint64_t *)(buf + 144)), y9;
    uint64x2_t x10 = vld1q_u64((const uint64_t *)(buf + 160)), y10;
    uint64x2_t x11 = vld1q_u64((const uint64_t *)(buf + 176)), y11;
    uint64x2_t k;
    {
      static const uint64_t CRC_ALIGN(16) k_[] = {0xa87ab8a8, 0xab7aff2a};
      k = vld1q_u64(k_);
    }
    x0 = veorq_u64((uint64x2_t){crc0, 0}, x0);
    buf += 192;
    len -= 192;
    /* Main loop. */
    while (len >= 192) {
      y0 = clmul_lo_e(x0, k, vld1q_u64((const uint64_t *)buf)),
      x0 = clmul_hi_e(x0, k, y0);
      y1 = clmul_lo_e(x1, k, vld1q_u64((const uint64_t *)(buf + 16))),
      x1 = clmul_hi_e(x1, k, y1);
      y2 = clmul_lo_e(x2, k, vld1q_u64((const uint64_t *)(buf + 32))),
      x2 = clmul_hi_e(x2, k, y2);
      y3 = clmul_lo_e(x3, k, vld1q_u64((const uint64_t *)(buf + 48))),
      x3 = clmul_hi_e(x3, k, y3);
      y4 = clmul_lo_e(x4, k, vld1q_u64((const uint64_t *)(buf + 64))),
      x4 = clmul_hi_e(x4, k, y4);
      y5 = clmul_lo_e(x5, k, vld1q_u64((const uint64_t *)(buf + 80))),
      x5 = clmul_hi_e(x5, k, y5);
      y6 = clmul_lo_e(x6, k, vld1q_u64((const uint64_t *)(buf + 96))),
      x6 = clmul_hi_e(x6, k, y6);
      y7 = clmul_lo_e(x7, k, vld1q_u64((const uint64_t *)(buf + 112))),
      x7 = clmul_hi_e(x7, k, y7);
      y8 = clmul_lo_e(x8, k, vld1q_u64((const uint64_t *)(buf + 128))),
      x8 = clmul_hi_e(x8, k, y8);
      y9 = clmul_lo_e(x9, k, vld1q_u64((const uint64_t *)(buf + 144))),
      x9 = clmul_hi_e(x9, k, y9);
      y10 = clmul_lo_e(x10, k, vld1q_u64((const uint64_t *)(buf + 160))),
      x10 = clmul_hi_e(x10, k, y10);
      y11 = clmul_lo_e(x11, k, vld1q_u64((const uint64_t *)(buf + 176))),
      x11 = clmul_hi_e(x11, k, y11);
      buf += 192;
      len -= 192;
    }
    /* Reduce x0 ... x11 to just x0. */
    {
      static const uint64_t CRC_ALIGN(16) k_[] = {0xf20c0dfe, 0x493c7d27};
      k = vld1q_u64(k_);
    }
    y0 = clmul_lo_e(x0, k, x1), x0 = clmul_hi_e(x0, k, y0);
    y2 = clmul_lo_e(x2, k, x3), x2 = clmul_hi_e(x2, k, y2);
    y4 = clmul_lo_e(x4, k, x5), x4 = clmul_hi_e(x4, k, y4);
    y6 = clmul_lo_e(x6, k, x7), x6 = clmul_hi_e(x6, k, y6);
    y8 = clmul_lo_e(x8, k, x9), x8 = clmul_hi_e(x8, k, y8);
    y10 = clmul_lo_e(x10, k, x11), x10 = clmul_hi_e(x10, k, y10);
    {
      static const uint64_t CRC_ALIGN(16) k_[] = {0x3da6d0cb, 0xba4fc28e};
      k = vld1q_u64(k_);
    }
    y0 = clmul_lo_e(x0, k, x2), x0 = clmul_hi_e(x0, k, y0);
    y4 = clmul_lo_e(x4, k, x6), x4 = clmul_hi_e(x4, k, y4);
    y8 = clmul_lo_e(x8, k, x10), x8 = clmul_hi_e(x8, k, y8);
    {
      static const uint64_t CRC_ALIGN(16) k_[] = {0x740eef02, 0x9e4addf8};
      k = vld1q_u64(k_);
    }
    y0 = clmul_lo_e(x0, k, x4), x0 = clmul_hi_e(x0, k, y0);
    x4 = x8;
    y0 = clmul_lo_e(x0, k, x4), x0 = clmul_hi_e(x0, k, y0);
    /* Reduce 128 bits to 32 bits, and multiply by x^32. */
    crc0 = __crc32cd(0, vgetq_lane_u64(x0, 0));
    crc0 = __crc32cd(crc0, vgetq_lane_u64(x0, 1));
  }
  for (; len >= 8; buf += 8, len -= 8) {
    crc0 = __crc32cd(crc0, *(const uint64_t *)buf);
  }
  for (; len; --len) {
    crc0 = __crc32cb(crc0, *buf++);
  }
  return crc0;
}

#elif defined(__x86_64__) && defined(__SSE4_2__)

/* Generated by https://github.com/corsix/fast-crc32/ using: */
/* ./generate -i sse -p crc32c -a v4s3x3 */
/* MIT licensed */

#include <nmmintrin.h>
#include <wmmintrin.h>

#define clmul_lo(a, b) (_mm_clmulepi64_si128((a), (b), 0))
#define clmul_hi(a, b) (_mm_clmulepi64_si128((a), (b), 17))

CRC_AINLINE __m128i clmul_scalar(uint32_t a, uint32_t b) {
  return _mm_clmulepi64_si128(_mm_cvtsi32_si128(a), _mm_cvtsi32_si128(b), 0);
}

CRC_EXPORT
uint32_t clmulr32(uint32_t a, uint32_t b) {
  __m128i aa = _mm_cvtsi32_si128(a), bb = _mm_cvtsi32_si128(b);
  __m128i j = _mm_slli_epi64(clmul_lo(aa, bb), 1);
  __m128i k;
  k = _mm_cvtsi64_si128(0x90d3d871bd4e27e2ull);
  __m128i l = clmul_lo(j, k);
  static const uint64_t CRC_ALIGN(16)
      k_[] = {0xfffffffe00000000ull, (uint64_t)-1ll};
  k = _mm_load_si128((const __m128i *)k_);
  __m128i n = _mm_and_si128(l, k);
  k = _mm_set1_epi64x(0x82f63b7880000000ull);
  __m128i hi = clmul_lo(n, k);
  __m128i shl = _mm_srli_si128(hi, 8);
  __m128i r = clmul_hi(n, k);
  return _mm_extract_epi32(_mm_xor_si128(r, shl), 0);
}

CRC_EXPORT uint32_t xnmodp(uint64_t n) /* x^n mod P, in log(n) time */ {
  uint64_t stack = ~(uint64_t)1;
  uint32_t acc, low;
  for (; n > 191; n = (n >> 1) - 16) {
    stack = (stack << 1) + (n & 1);
  }
  stack = ~stack;
  acc = ((uint32_t)0x80000000) >> (n & 31);
  for (n >>= 5; n; --n) {
    acc = _mm_crc32_u32(acc, 0);
  }
  while ((low = stack & 1), stack >>= 1) {
    __m128i x = _mm_cvtsi32_si128(acc);
    uint64_t y = _mm_cvtsi128_si64(_mm_clmulepi64_si128(x, x, 0));
    acc = _mm_crc32_u64(0, y << low);
  }
  return acc;
}

CRC_AINLINE __m128i crc_shift(uint32_t crc, size_t nbytes) {
  return clmul_scalar(crc, xnmodp(nbytes * 8 - 33));
}

CRC_EXPORT uint32_t crc32c(const char *buf, size_t len) {
  uint32_t crc0 = 0;
  for (; len && ((uintptr_t)buf & 7); --len) {
    crc0 = _mm_crc32_u8(crc0, *buf++);
  }
  if (((uintptr_t)buf & 8) && len >= 8) {
    crc0 = _mm_crc32_u64(crc0, *(const uint64_t *)buf);
    buf += 8;
    len -= 8;
  }
  if (len >= 144) {
    size_t blk = (len - 8) / 136;
    size_t klen = blk * 24;
    const char *buf2 = buf + 0;
    uint32_t crc1 = 0;
    uint32_t crc2 = 0;
    __m128i vc0;
    __m128i vc1;
    uint64_t vc;
    /* First vector chunk. */
    __m128i x0 = _mm_loadu_si128((const __m128i *)buf2), y0;
    __m128i x1 = _mm_loadu_si128((const __m128i *)(buf2 + 16)), y1;
    __m128i x2 = _mm_loadu_si128((const __m128i *)(buf2 + 32)), y2;
    __m128i x3 = _mm_loadu_si128((const __m128i *)(buf2 + 48)), y3;
    __m128i k;
    k = _mm_setr_epi32(0x740eef02, 0, 0x9e4addf8, 0);
    x0 = _mm_xor_si128(_mm_cvtsi32_si128(crc0), x0);
    crc0 = 0;
    buf2 += 64;
    len -= 136;
    buf += blk * 64;
    /* Main loop. */
    while (len >= 144) {
      y0 = clmul_lo(x0, k), x0 = clmul_hi(x0, k);
      y1 = clmul_lo(x1, k), x1 = clmul_hi(x1, k);
      y2 = clmul_lo(x2, k), x2 = clmul_hi(x2, k);
      y3 = clmul_lo(x3, k), x3 = clmul_hi(x3, k);
      y0 = _mm_xor_si128(y0, _mm_loadu_si128((const __m128i *)buf2)),
      x0 = _mm_xor_si128(x0, y0);
      y1 = _mm_xor_si128(y1, _mm_loadu_si128((const __m128i *)(buf2 + 16))),
      x1 = _mm_xor_si128(x1, y1);
      y2 = _mm_xor_si128(y2, _mm_loadu_si128((const __m128i *)(buf2 + 32))),
      x2 = _mm_xor_si128(x2, y2);
      y3 = _mm_xor_si128(y3, _mm_loadu_si128((const __m128i *)(buf2 + 48))),
      x3 = _mm_xor_si128(x3, y3);
      crc0 = _mm_crc32_u64(crc0, *(const uint64_t *)buf);
      crc1 = _mm_crc32_u64(crc1, *(const uint64_t *)(buf + klen));
      crc2 = _mm_crc32_u64(crc2, *(const uint64_t *)(buf + klen * 2));
      crc0 = _mm_crc32_u64(crc0, *(const uint64_t *)(buf + 8));
      crc1 = _mm_crc32_u64(crc1, *(const uint64_t *)(buf + klen + 8));
      crc2 = _mm_crc32_u64(crc2, *(const uint64_t *)(buf + klen * 2 + 8));
      crc0 = _mm_crc32_u64(crc0, *(const uint64_t *)(buf + 16));
      crc1 = _mm_crc32_u64(crc1, *(const uint64_t *)(buf + klen + 16));
      crc2 = _mm_crc32_u64(crc2, *(const uint64_t *)(buf + klen * 2 + 16));
      buf += 24;
      buf2 += 64;
      len -= 136;
    }
    /* Reduce x0 ... x3 to just x0. */
    k = _mm_setr_epi32(0xf20c0dfe, 0, 0x493c7d27, 0);
    y0 = clmul_lo(x0, k), x0 = clmul_hi(x0, k);
    y2 = clmul_lo(x2, k), x2 = clmul_hi(x2, k);
    y0 = _mm_xor_si128(y0, x1), x0 = _mm_xor_si128(x0, y0);
    y2 = _mm_xor_si128(y2, x3), x2 = _mm_xor_si128(x2, y2);
    k = _mm_setr_epi32(0x3da6d0cb, 0, 0xba4fc28e, 0);
    y0 = clmul_lo(x0, k), x0 = clmul_hi(x0, k);
    y0 = _mm_xor_si128(y0, x2), x0 = _mm_xor_si128(x0, y0);
    /* Final scalar chunk. */
    crc0 = _mm_crc32_u64(crc0, *(const uint64_t *)buf);
    crc1 = _mm_crc32_u64(crc1, *(const uint64_t *)(buf + klen));
    crc2 = _mm_crc32_u64(crc2, *(const uint64_t *)(buf + klen * 2));
    crc0 = _mm_crc32_u64(crc0, *(const uint64_t *)(buf + 8));
    crc1 = _mm_crc32_u64(crc1, *(const uint64_t *)(buf + klen + 8));
    crc2 = _mm_crc32_u64(crc2, *(const uint64_t *)(buf + klen * 2 + 8));
    crc0 = _mm_crc32_u64(crc0, *(const uint64_t *)(buf + 16));
    crc1 = _mm_crc32_u64(crc1, *(const uint64_t *)(buf + klen + 16));
    crc2 = _mm_crc32_u64(crc2, *(const uint64_t *)(buf + klen * 2 + 16));
    buf += 24;
    vc0 = crc_shift(crc0, klen * 2 + 8);
    vc1 = crc_shift(crc1, klen + 8);
    vc = _mm_extract_epi64(_mm_xor_si128(vc0, vc1), 0);
    /* Reduce 128 bits to 32 bits, and multiply by x^32. */
    vc ^= _mm_extract_epi64(
        crc_shift(_mm_crc32_u64(_mm_crc32_u64(0, _mm_extract_epi64(x0, 0)),
                                _mm_extract_epi64(x0, 1)),
                  klen * 3 + 8),
        0);
    /* Final 8 bytes. */
    buf += klen * 2;
    crc0 = crc2;
    crc0 = _mm_crc32_u64(crc0, *(const uint64_t *)buf ^ vc), buf += 8;
    len -= 8;
  }
  for (; len >= 8; buf += 8, len -= 8) {
    crc0 = _mm_crc32_u64(crc0, *(const uint64_t *)buf);
  }
  for (; len; --len) {
    crc0 = _mm_crc32_u8(crc0, *buf++);
  }
  return crc0;
}

#endif

#endif
