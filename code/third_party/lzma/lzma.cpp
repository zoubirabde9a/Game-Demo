/* LZMA: the encoder and decoder of the LZMA SDK 23.01 by Igor Pavlov
   (https://www.7-zip.org/sdk.html), placed in the public domain. Only the
   files they need are here, built single-threaded (Z7_ST) into whatever
   includes this file. Used by server/replay.cpp to compress replays.

   One change from the SDK: LzmaEnc.c, in LzmaEnc_Create, casts the void*
   it returns, so the file builds as C++ in this repo's unity builds.
   The .c files' own macros (Align, Literal...) are undefined after them
   (lzma_undefs.inc), or they would rewrite the game code built after. */

#if defined(_MSC_VER)
#pragma warning(push, 0)
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#endif

#define Z7_ST
#include "CpuArch.c"
#include "LzFind.c"
#include "LzFindOpt.c"
#include "LzmaDec.c"
#include "LzmaEnc.c"
#include "lzma_undefs.inc"

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
