/* Ashen Wastes: an endless burnt plain. Grey ash flats over black basalt,
   lava rivers that wind for miles with scorched basalt banks, and basalt
   crags on the high ground, chasms that drop into nothing, and the odd
   hot spring. Dead trees and boulders are the only cover,
   apart from abandoned camps: crates inside a broken fence. The ground
   climbs in basalt mesas, three steps a tier, mostly ringed by cliffs
   with a stair here and there; along stretches of the lava a ridge two
   steps high walls the river in. Generated for any tile from the seed.
   Fire monsters rule here. */
#if defined(MAP_NAME_PASS)
MAP(Wastes)
#else

#include "map_shapes.h"

internal terrain_kind
GenerateWastes(map_def *Map, i32 X, i32 Y)
{
    if (InSpawnClearing(X, Y))
    {
        return TerrainKind_Ash;
    }
    i32 Height = FractalNoise(Map->Seed, X, Y, 40, 3);
    i32 River = RidgeDistance(FractalNoise(Map->Seed + 3, X, Y, 56, 2));
    i32 Rockiness = FractalNoise(Map->Seed + 9, X, Y, 20, 2);

    terrain_kind Result = TerrainKind_Ash;
    if (River < NOISE_PERCENT(5))
    {
        Result = TerrainKind_Lava;
    }
    else if (River < NOISE_PERCENT(9))
    {
        Result = TerrainKind_Basalt;
    }
    else if (Height > NOISE_PERCENT(68))
    {
        Result = TerrainKind_BasaltWall;
    }
    else if (FractalNoise(Map->Seed + 31, X, Y, 10, 1) > NOISE_PERCENT(90))
    {
        Result = TerrainKind_Pit;
    }
    else if (FractalNoise(Map->Seed + 33, X, Y, 10, 1) > NOISE_PERCENT(94))
    {
        Result = TerrainKind_Spring;
    }
    else if (Rockiness > NOISE_PERCENT(60))
    {
        // NOTE(zoubir): ley runes, few and far between, on the basalt
        Result = TileRoll(Map, X, Y, 11) < 2 ? TerrainKind_Rune : TerrainKind_Basalt;
    }
    return Result;
}

// NOTE(zoubir): the same height and river fields as the ground: mesas
// rise toward the basalt walls, and the lava runs at the bottom
internal i32
GenerateWastesElevation(map_def *Map, i32 X, i32 Y)
{
    if (InSpawnClearing(X, Y))
    {
        return 0;
    }
    i32 River = RidgeDistance(FractalNoise(Map->Seed + 3, X, Y, 56, 2));
    if (River < NOISE_PERCENT(9))
    {
        return 0;
    }
    i32 Height = FractalNoise(Map->Seed, X, Y, 40, 3);
    bool32 Stairs = FractalNoise(Map->Seed + 23, X, Y, 24, 2) > NOISE_PERCENT(58);
    i32 Result = TerraceSteps(Height, NOISE_PERCENT(47), NOISE_PERCENT(10), NOISE_PERCENT(5),
                              3, Stairs);
    if (River < NOISE_PERCENT(15) &&
        FractalNoise(Map->Seed + 27, X, Y, 30, 2) > NOISE_PERCENT(52))
    {
        Result = Maximum(Result, Stairs ? 1 : 2);
    }
    Result = Minimum(Result, 8);
    Result = FlattenNearSpawnAndLandmarks(Map, X, Y, Result);
    return Result;
}

internal terrain_prop
PlaceWastesProp(map_def *Map, i32 X, i32 Y, terrain_kind Ground)
{
    if (InSpawnClearing(X, Y) || Ground == TerrainKind_Lava)
    {
        return TerrainProp_None;
    }
    // NOTE(zoubir): an abandoned camp, here and there: crates in the
    // middle, a ring of fence with gaps around them
    i32 Camp = 0;
    if (FractalNoise(Map->Seed + 43, X, Y, 64, 1) > NOISE_PERCENT(62))
    {
        Camp = FractalNoise(Map->Seed + 41, X, Y, 16, 1);
    }
    if (Camp > NOISE_PERCENT(86))
    {
        return TileRoll(Map, X, Y, 8) < 12 ? TerrainProp_Crate : TerrainProp_None;
    }
    if (Camp > NOISE_PERCENT(83))
    {
        return TileRoll(Map, X, Y, 9) < 65 ? TerrainProp_Fence : TerrainProp_None;
    }
    u32 Roll = TileRoll(Map, X, Y, 2);
    terrain_prop Result = TerrainProp_None;
    if (Ground == TerrainKind_Ash && Roll < 3)
    {
        Result = TerrainProp_DeadTree;
    }
    else if (Ground == TerrainKind_Basalt && Roll < 4)
    {
        Result = TerrainProp_Boulder;
    }
    return Result;
}

internal void
DefineMap_Wastes(map_def *Map)
{
    Map->Name = "Ashen Wastes";
    Map->Kind = MapKind_Infinite;
    Map->Seed = 6661;
    Map->Generate = GenerateWastes;
    Map->PlaceProp = PlaceWastesProp;
    Map->GenerateElevation = GenerateWastesElevation;
    Map->Outside = TerrainKind_Ash;
    Map->SpawnCount = 1;
    Map->MonsterWeight[MonsterKind_Imp] = 3.f;
    Map->MonsterWeight[MonsterKind_Brute] = 2.f;
    Map->MonsterWeight[MonsterKind_Warlord] = 3.f;
    Map->MonsterWeight[MonsterKind_Lurker] = 2.f;
    Map->MonsterWeight[MonsterKind_Toad] = 0.f;
    Map->MonsterWeight[MonsterKind_Spider] = 0.f;
    Map->MonsterWeight[MonsterKind_Slime] = 0.f;
}

#endif
