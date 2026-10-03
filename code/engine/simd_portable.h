#if !defined(ENGINE_SIMD_PORTABLE_H)
#define ENGINE_SIMD_PORTABLE_H
/* The SSE operations the sound mixer (audio.cpp) uses, in plain C, for
   CPUs without SSE such as the ARM servers the game server runs on.
   app_platform.h includes this instead of <x86intrin.h> on those CPUs.
   Same types and names, same lane layout (the mixer reads lanes through
   an i32 pointer), same rounding and saturation as the x86 instructions.
   The compiler turns the 4-wide loops into NEON on ARM. */

#include <math.h>

struct alignas(16) __m128 { float F[4]; };
struct alignas(16) __m128i
{
    union
    {
        i32 I32[4];
        i16 I16[8];
    };
};

inline __m128 _mm_set1_ps(float A) { __m128 R; for (int I = 0; I < 4; ++I) R.F[I] = A; return R; }
inline __m128 _mm_setr_ps(float A, float B, float C, float D) { __m128 R = {{A, B, C, D}}; return R; }
inline __m128 _mm_load_ps(float const *P) { __m128 R; for (int I = 0; I < 4; ++I) R.F[I] = P[I]; return R; }
inline void _mm_store_ps(float *P, __m128 A) { for (int I = 0; I < 4; ++I) P[I] = A.F[I]; }

inline __m128 _mm_add_ps(__m128 A, __m128 B) { for (int I = 0; I < 4; ++I) A.F[I] += B.F[I]; return A; }
inline __m128 _mm_sub_ps(__m128 A, __m128 B) { for (int I = 0; I < 4; ++I) A.F[I] -= B.F[I]; return A; }
inline __m128 _mm_mul_ps(__m128 A, __m128 B) { for (int I = 0; I < 4; ++I) A.F[I] *= B.F[I]; return A; }
inline __m128 _mm_div_ps(__m128 A, __m128 B) { for (int I = 0; I < 4; ++I) A.F[I] /= B.F[I]; return A; }

// x86 returns 0x80000000 for NaN and for values outside the i32 range.
inline i32
SimdFloatToI32(float F)
{
    if (!(F > -2147483648.0f && F < 2147483648.0f)) return (i32)0x80000000u;
    return (i32)F;
}

// _mm_cvtps_epi32 rounds to nearest, ties to even (the default rounding mode).
inline __m128i
_mm_cvtps_epi32(__m128 A)
{
    __m128i R;
    for (int I = 0; I < 4; ++I) R.I32[I] = SimdFloatToI32(nearbyintf(A.F[I]));
    return R;
}

// _mm_cvttps_epi32 truncates toward zero.
inline __m128i
_mm_cvttps_epi32(__m128 A)
{
    __m128i R;
    for (int I = 0; I < 4; ++I) R.I32[I] = SimdFloatToI32(A.F[I]);
    return R;
}

inline __m128
_mm_cvtepi32_ps(__m128i A)
{
    __m128 R;
    for (int I = 0; I < 4; ++I) R.F[I] = (float)A.I32[I];
    return R;
}

// a0 b0 a1 b1
inline __m128i
_mm_unpacklo_epi32(__m128i A, __m128i B)
{
    __m128i R;
    R.I32[0] = A.I32[0]; R.I32[1] = B.I32[0]; R.I32[2] = A.I32[1]; R.I32[3] = B.I32[1];
    return R;
}

// a2 b2 a3 b3
inline __m128i
_mm_unpackhi_epi32(__m128i A, __m128i B)
{
    __m128i R;
    R.I32[0] = A.I32[2]; R.I32[1] = B.I32[2]; R.I32[2] = A.I32[3]; R.I32[3] = B.I32[3];
    return R;
}

inline i16
SimdSaturateI16(i32 V)
{
    return (i16)(V > 32767 ? 32767 : (V < -32768 ? -32768 : V));
}

// The four i32 of A then the four of B, each clamped to i16.
inline __m128i
_mm_packs_epi32(__m128i A, __m128i B)
{
    __m128i R;
    for (int I = 0; I < 4; ++I)
    {
        R.I16[I] = SimdSaturateI16(A.I32[I]);
        R.I16[I + 4] = SimdSaturateI16(B.I32[I]);
    }
    return R;
}

#endif
