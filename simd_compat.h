#ifndef JPEG2PNG_SIMD_COMPAT_H
#define JPEG2PNG_SIMD_COMPAT_H

// Maps the few SSE intrinsics used by compute_simd_step.c to NEON on aarch64.
#if defined(__aarch64__) || defined(__ARM_NEON)
#include <arm_neon.h>

typedef float32x4_t __m128;

#define _mm_empty() ((void)0)
static inline __m128 _mm_load_ps(const float *p) { return vld1q_f32(p); }
static inline __m128 _mm_loadu_ps(const float *p) { return vld1q_f32(p); }
static inline void _mm_store_ps(float *p, __m128 v) { vst1q_f32(p, v); }
static inline void _mm_storeu_ps(float *p, __m128 v) { vst1q_f32(p, v); }
static inline __m128 _mm_set_ps1(float x) { return vdupq_n_f32(x); }
static inline __m128 _mm_sqrt_ps(__m128 v) { return vsqrtq_f32(v); }
static inline __m128 _mm_max_ps(__m128 a, __m128 b) { return vmaxq_f32(a, b); }
static inline __m128 _mm_min_ps(__m128 a, __m128 b) { return vminq_f32(a, b); }
static inline __m128 _mm_or_ps(__m128 a, __m128 b) {
        return vreinterpretq_f32_u32(vorrq_u32(vreinterpretq_u32_f32(a), vreinterpretq_u32_f32(b)));
}
static inline __m128 _mm_and_ps(__m128 a, __m128 b) {
        return vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(a), vreinterpretq_u32_f32(b)));
}
static inline __m128 _mm_cmpeq_ps(__m128 a, __m128 b) { return vreinterpretq_f32_u32(vceqq_f32(a, b)); }
// converts 4 int16 at p to floats
static inline __m128 jpeg2png_cvt_i16x4(const int16_t *p) { return vcvtq_f32_s32(vmovl_s16(vld1_s16(p))); }
#define _mm_cvtpi16_ps_ptr(p) jpeg2png_cvt_i16x4((const int16_t *)(p))
#else
#include <xmmintrin.h>
#define _mm_cvtpi16_ps_ptr(p) _mm_cvtpi16_ps(*(__m64 *)(p))
#endif

#endif
