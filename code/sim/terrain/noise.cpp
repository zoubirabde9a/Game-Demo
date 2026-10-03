/* Integer noise for terrain generation. No floats anywhere: the server
   (g++ on Linux) and the clients (MSVC on Windows) must compute the same
   ground bit for bit, and float rounding can differ between them right at
   a threshold. Values are fixed point with NOISE_ONE as 1.0. */

#define NOISE_SHIFT 16
#define NOISE_ONE (1 << NOISE_SHIFT)

// NOTE(zoubir): floor division that also rounds negative numbers down;
// C++ integer division rounds toward zero
inline i32
FloorDiv(i32 Value, i32 Divisor)
{
    i32 Result = Value / Divisor;
    if ((Value % Divisor) != 0 && ((Value < 0) != (Divisor < 0)))
    {
        Result -= 1;
    }
    return Result;
}

inline i32
FloorMod(i32 Value, i32 Divisor)
{
    i32 Result = Value - FloorDiv(Value, Divisor) * Divisor;
    return Result;
}

// NOTE(zoubir): a 32-bit avalanche hash of a lattice point and a seed
inline u32
HashLattice(u32 Seed, i32 X, i32 Y)
{
    u32 H = Seed * 0x9E3779B9u;
    H ^= (u32)X * 0x85EBCA6Bu;
    H = (H << 13) | (H >> 19);
    H ^= (u32)Y * 0xC2B2AE35u;
    H ^= H >> 16;
    H *= 0x7FEB352Du;
    H ^= H >> 15;
    H *= 0x846CA68Bu;
    H ^= H >> 16;
    return H;
}

// NOTE(zoubir): 0..NOISE_ONE - 1 at a lattice point
inline i32
LatticeValue(u32 Seed, i32 X, i32 Y)
{
    i32 Result = (i32)(HashLattice(Seed, X, Y) >> (32 - NOISE_SHIFT));
    return Result;
}

// NOTE(zoubir): 3t^2 - 2t^3 in fixed point, for smooth interpolation
inline i32
SmoothStepFixed(i32 T)
{
    i64 T2 = ((i64)T * T) >> NOISE_SHIFT;
    i64 T3 = (T2 * T) >> NOISE_SHIFT;
    i32 Result = (i32)(3 * T2 - 2 * T3);
    return Result;
}

inline i32
LerpFixed(i32 A, i32 B, i32 T)
{
    i32 Result = A + (i32)((((i64)(B - A)) * T) >> NOISE_SHIFT);
    return Result;
}

// NOTE(zoubir): smooth value noise over a lattice of CellSize tiles,
// 0..NOISE_ONE at any tile
internal i32
ValueNoise(u32 Seed, i32 X, i32 Y, i32 CellSize)
{
    i32 CellX = FloorDiv(X, CellSize);
    i32 CellY = FloorDiv(Y, CellSize);
    i32 TX = SmoothStepFixed((FloorMod(X, CellSize) << NOISE_SHIFT) / CellSize);
    i32 TY = SmoothStepFixed((FloorMod(Y, CellSize) << NOISE_SHIFT) / CellSize);
    i32 A = LatticeValue(Seed, CellX, CellY);
    i32 B = LatticeValue(Seed, CellX + 1, CellY);
    i32 C = LatticeValue(Seed, CellX, CellY + 1);
    i32 D = LatticeValue(Seed, CellX + 1, CellY + 1);
    i32 Result = LerpFixed(LerpFixed(A, B, TX), LerpFixed(C, D, TX), TY);
    return Result;
}

// NOTE(zoubir): Octaves layers of value noise, each half the cell size and
// half the weight of the one before, renormalized to 0..NOISE_ONE
internal i32
FractalNoise(u32 Seed, i32 X, i32 Y, i32 CellSize, u32 Octaves)
{
    i64 Sum = 0;
    i64 WeightSum = 0;
    i32 Weight = 256;
    for(u32 Octave = 0; Octave < Octaves && CellSize >= 1; Octave++)
    {
        Sum += (i64)ValueNoise(Seed + Octave * 101u, X, Y, CellSize) * Weight;
        WeightSum += Weight;
        Weight /= 2;
        CellSize /= 2;
    }
    i32 Result = (i32)(Sum / WeightSum);
    return Result;
}

// NOTE(zoubir): 0 on the noise's middle contour, rising to NOISE_ONE away
// from it; thin bands where it is small make rivers and paths
inline i32
RidgeDistance(i32 Noise)
{
    i32 Result = Noise - NOISE_ONE / 2;
    if (Result < 0)
    {
        Result = -Result;
    }
    Result *= 2;
    return Result;
}

// NOTE(zoubir): percent-of-NOISE_ONE helper for readable thresholds
#define NOISE_PERCENT(P) ((NOISE_ONE / 100) * (P))
