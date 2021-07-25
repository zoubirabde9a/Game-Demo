#if !defined(APP_DEFS_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Zoubir $
   ======================================================================== */

#define APP_DEFS_H

#include <stdint.h>

// TODO(zoubir): Remove Those
#if APP_DEV
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#endif

#define Pi32 3.14159265359f

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef int32_t bool32;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define internal static
#define local_persist static
#define global_variable static

#if APP_DEV
#define Assert(expression) if (!(expression)) {*(volatile int *)0 = 0;}
#define InvalidCodePath Assert(!"InvalidCodePath")
#else
#define Assert(expression)
#endif

#define Kilobytes(value) ((value) * 1024ll)
#define Megabytes(value) (Kilobytes(value) * 1024ll)
#define Gigabytes(value) (Megabytes(value) * 1024ll)
#define Terabytes(value) (Gigabytes(value) * 1024ll)

#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))

#define Align4(Value) ((Value + 3) & ~3)
#define Align8(Value) ((Value + 7) & ~7)
#define Align16(Value) ((Value + 15) & ~15)

#define OffsetOf(Type, Element) ((size_t)&(((Type *)0)->Element))


#endif
