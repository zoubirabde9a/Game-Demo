/* Map shapes: the pieces the procedural maps share to raise ground, all
   integer math like the rest of generation. A map's GenerateElevation
   turns a noise value into steps with TerraceSteps, then caps the result
   with FlattenNearSpawnAndLandmarks so players spawn on flat ground and
   landmarks never sit in a pit. Included by each map file that uses it. */
#if !defined(MAP_SHAPES_H)
#define MAP_SHAPES_H

// NOTE(zoubir): in landmarks.cpp, included after the maps
internal i32 LandmarkClearance(map_def *Map, i32 X, i32 Y);

/* Steps of ground for a noise value. Above Start, every Band of noise is
   one tier StepsPerTier steps above the one below. On its own a tier ends
   in a cliff (StepsPerTier steps: a jump). Where Stairs is set, the last
   Rise of noise before each tier edge climbs one step at a time instead,
   so that stretch of the edge can be walked up. */
internal i32
TerraceSteps(i32 Noise, i32 Start, i32 Band, i32 Rise, i32 StepsPerTier,
             bool32 Stairs)
{
    i32 Above = Noise - Start;
    if (Above < 0)
    {
        return 0;
    }
    i32 Tier = Above / Band;
    i32 Into = Above % Band;
    i32 Result = Tier * StepsPerTier;
    if (Stairs && Into > Band - Rise)
    {
        Result += ((Into - (Band - Rise)) * StepsPerTier) / Rise;
        Result = Minimum(Result, (Tier + 1) * StepsPerTier - 1);
    }
    return Result;
}

inline i32
IntegerSquareRoot(i32 Value)
{
    i32 Result = 0;
    while ((Result + 1) * (Result + 1) <= Value)
    {
        Result++;
    }
    return Result;
}

/* Ground rises at most one step per tile away from the spawn clearing and
   from a landmark, so both sit flat and can be walked out of. */
internal i32
FlattenNearSpawnAndLandmarks(map_def *Map, i32 X, i32 Y, i32 Steps)
{
    if (Steps <= 0)
    {
        return Steps;
    }
    i32 Reach = MAP_SPAWN_CLEARING + ELEVATION_MAX_STEPS;
    i32 DistanceSq = X * X + Y * Y;
    if (DistanceSq <= Reach * Reach)
    {
        i32 Cap = IntegerSquareRoot(DistanceSq) - MAP_SPAWN_CLEARING;
        Steps = Minimum(Steps, Maximum(Cap, 0));
    }
    Steps = Minimum(Steps, LandmarkClearance(Map, X, Y));
    return Steps;
}

#endif
