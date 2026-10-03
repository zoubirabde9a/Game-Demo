/* SIMD tests: the SSE operations the sound mixer uses give the expected
   results. On x86 this checks the real instructions; on ARM it checks
   engine/simd_portable.h. The expected values are the same, so passing on
   both means the portable versions match SSE. Run by test.bat; on ARM
   build it with g++ like the other tests. */

#include <stdio.h>
#include "../app_platform.h"

global_variable int TestFailures;
global_variable int TestChecks;

#define Check(Expression) CheckImpl((Expression) != 0, #Expression, __LINE__)

internal void
CheckImpl(bool Passed, const char *Expression, int Line)
{
    TestChecks++;
    if (!Passed)
    {
        TestFailures++;
        printf("  FAILED simd_tests.cpp(%d): %s\n", Line, Expression);
    }
}

internal i32 Lane(__m128i V, int Index) { return ((i32 *)&V)[Index]; }
internal i16 Lane16(__m128i V, int Index) { return ((i16 *)&V)[Index]; }
internal float LaneF(__m128 V, int Index) { return ((float *)&V)[Index]; }

int
main()
{
    // Rounding: to nearest, halves to even.
    __m128i Rounded = _mm_cvtps_epi32(_mm_setr_ps(2.5f, 3.5f, -2.5f, 1.4f));
    Check(Lane(Rounded, 0) == 2 && Lane(Rounded, 1) == 4);
    Check(Lane(Rounded, 2) == -2 && Lane(Rounded, 3) == 1);

    // Out of range and NaN give 0x80000000.
    float NotANumber = 0.0f;
    NotANumber = NotANumber / NotANumber;
    __m128i Invalid = _mm_cvtps_epi32(_mm_setr_ps(3e9f, -3e9f, NotANumber, -1.0f));
    Check(Lane(Invalid, 0) == (i32)0x80000000u && Lane(Invalid, 1) == (i32)0x80000000u);
    Check(Lane(Invalid, 2) == (i32)0x80000000u && Lane(Invalid, 3) == -1);

    // Truncation toward zero, and back to float.
    __m128i Truncated = _mm_cvttps_epi32(_mm_setr_ps(2.9f, -2.9f, 0.5f, 7.0f));
    Check(Lane(Truncated, 0) == 2 && Lane(Truncated, 1) == -2);
    Check(Lane(Truncated, 2) == 0 && Lane(Truncated, 3) == 7);
    __m128 Back = _mm_cvtepi32_ps(Truncated);
    Check(LaneF(Back, 1) == -2.0f && LaneF(Back, 3) == 7.0f);

    // Interleaving left and right channels, as the mixer's output does.
    __m128i L = _mm_cvtps_epi32(_mm_setr_ps(1, 2, 3, 4));
    __m128i R = _mm_cvtps_epi32(_mm_setr_ps(10, 20, 30, 40));
    __m128i Low = _mm_unpacklo_epi32(L, R);
    __m128i High = _mm_unpackhi_epi32(L, R);
    Check(Lane(Low, 0) == 1 && Lane(Low, 1) == 10 && Lane(Low, 2) == 2 && Lane(Low, 3) == 20);
    Check(Lane(High, 0) == 3 && Lane(High, 1) == 30 && Lane(High, 2) == 4 && Lane(High, 3) == 40);

    // Packing to 16 bits saturates, A's lanes then B's.
    __m128i Loud = _mm_cvtps_epi32(_mm_setr_ps(40000, -40000, 5, -5));
    __m128i Packed = _mm_packs_epi32(Loud, Low);
    Check(Lane16(Packed, 0) == 32767 && Lane16(Packed, 1) == -32768);
    Check(Lane16(Packed, 2) == 5 && Lane16(Packed, 3) == -5);
    Check(Lane16(Packed, 4) == 1 && Lane16(Packed, 5) == 10 && Lane16(Packed, 7) == 20);

    // Arithmetic, aligned load and store.
    __m128 A = _mm_setr_ps(1, 2, 3, 4);
    __m128 B = _mm_set1_ps(2);
    __m128 Sum = _mm_add_ps(_mm_mul_ps(A, B), _mm_div_ps(_mm_sub_ps(A, B), B));
    Check(LaneF(Sum, 0) == 1.5f && LaneF(Sum, 3) == 9.0f);
    __m128 Stored;
    _mm_store_ps((float *)&Stored, Sum);
    __m128 Loaded = _mm_load_ps((float *)&Stored);
    Check(LaneF(Loaded, 2) == LaneF(Sum, 2));

    printf("simd tests (%s): %d checks, %d failed\n",
#if defined(APP_PORTABLE_SIMD)
           "portable",
#else
           "native SSE",
#endif
           TestChecks, TestFailures);
    return TestFailures ? 1 : 0;
}
