#if !defined(INTRINSICS_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Zoubir $
   ======================================================================== */

#define INTRINSICS_H
#include <math.h>

inline u32
CeilFloatToUInt32(float A)
{
    u32 Result = (u32)ceilf(A);
    return Result;
}

#if COMPILER_MSVC

#define CompletePreviousWritesBeforeFutureWrites _WriteBarrier()

inline u32
AtomicCompareExchangeU32(u32 volatile *Value,
                                u32 NewValue,
                                u32 Expected)
{
    u32 Result =
        _InterlockedCompareExchange((long volatile *)Value,
                                   NewValue, Expected);
    return Result;
}
#elif COMPILER_GCC
#define CompletePreviousWritesBeforeFutureWrites asm volatile("" ::: "memory")

inline u32
AtomicCompareExchangeU32(u32 volatile *Value,
                                u32 ExpectedValue,
                                u32 NewValue)
{
    u32 Result =
        __sync_val_compare_and_swap((long volatile *)Value,
                                   ExpectedValue, NewValue);
    return Result;
}

#elif COMPILER_EMSCRIPTEN
#define CompletePreviousWritesBeforeFutureWrites asm volatile("" ::: "memory")

inline u32
AtomicCompareExchangeU32(u32 volatile *Value,
                                u32 ExpectedValue,
                                u32 NewValue)
{
    u32 Result =
        __sync_val_compare_and_swap((long volatile *)Value,
                                   ExpectedValue, NewValue);
    return Result;
}
#endif

inline float
Sin(float Angle)
{
    float Result = sinf(Angle);
    return Result;
}

inline float
Cos(float Angle)
{
    float Result = cosf(Angle);
    return Result;
}

inline float
ATan2(float Y, float X)
{
    float Result = atan2f(Y, X);
    return Result;
}

inline float
Tan(float X)
{
    float Result = tanf(X);
    return Result;
}

inline u32
FloorFloatToU32(float Value)
{
    u32 Result = (u32)floorf(Value);
    return Result;    
}

inline i32
RoundFloatToI32(float Value)
{
    i32 Result = (i32)roundf(Value);
    return Result;                             
}

inline u32
RoundFloatToU32(float Value)
{
    u32 Result = (u32)roundf(Value);
    return Result;                             
}

#endif
