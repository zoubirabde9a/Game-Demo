/* Verdant Wilds: endless green country. Rolling meadows broken by ponds
   and lakes, rocky outcrops on the high ground, muddy hollows where it is
   wet and stinking bogs at their hearts, bramble thickets, a sinkhole
   here and there, healing springs, and worn dirt trails that wander
   between them with a ley rune glowing on them now and then. The rocky
   crowns of the hills are rough but open ground. Trees gather in
   groves where the ground is damp; old paddocks keep broken fences and
   fallen logs. The ground rises in knolls one step up, then in grassy
   plateaus three steps higher: their edges are cliffs to jump, except
   where a stair climbs them one step at a time. Generated for any tile
   from the seed. */
#if defined(MAP_NAME_PASS)
MAP(Wilds)
#else

#include "map_shapes.h"

internal terrain_kind
GenerateWilds(map_def *Map, i32 X, i32 Y)
{
    if (InSpawnClearing(X, Y))
    {
        return TerrainKind_Grass;
    }
    i32 Height = FractalNoise(Map->Seed, X, Y, 32, 3);
    i32 Damp = FractalNoise(Map->Seed + 7, X, Y, 24, 2);
    i32 Trail = RidgeDistance(FractalNoise(Map->Seed + 13, X, Y, 64, 2));

    i32 Thorns = FractalNoise(Map->Seed + 41, X, Y, 12, 1);
    i32 Sink = FractalNoise(Map->Seed + 47, X, Y, 9, 1);
    i32 Spring = FractalNoise(Map->Seed + 43, X, Y, 10, 1);

    terrain_kind Result = TerrainKind_Grass;
    if (Height < NOISE_PERCENT(33))
    {
        Result = TerrainKind_DeepWater;
    }
    else if (Height < NOISE_PERCENT(37))
    {
        Result = TerrainKind_ShallowWater;
    }
    else if (Height > NOISE_PERCENT(73))
    {
        Result = TerrainKind_Rock;
    }
    else if (Trail < NOISE_PERCENT(4))
    {
        // NOTE(zoubir): now and then a ley rune glows on a trail
        Result = TileRoll(Map, X, Y, 9) < 3 ? TerrainKind_Rune : TerrainKind_Dirt;
    }
    else if (Sink > NOISE_PERCENT(91))
    {
        Result = TerrainKind_Pit;
    }
    else if (Spring > NOISE_PERCENT(92) && Height < NOISE_PERCENT(60))
    {
        Result = TerrainKind_Spring;
    }
    else if (Damp > NOISE_PERCENT(72) && Height < NOISE_PERCENT(46))
    {
        Result = TerrainKind_Bog;
    }
    else if (Damp > NOISE_PERCENT(62) && Height < NOISE_PERCENT(50))
    {
        Result = TerrainKind_Mud;
    }
    else if (Thorns > NOISE_PERCENT(80))
    {
        Result = TerrainKind_Bramble;
    }
    return Result;
}

// NOTE(zoubir): the same height field as the ground, so lakes lie low and
// the rock outcrops crown the plateaus
internal i32
GenerateWildsElevation(map_def *Map, i32 X, i32 Y)
{
    if (InSpawnClearing(X, Y))
    {
        return 0;
    }
    i32 Height = FractalNoise(Map->Seed, X, Y, 32, 3);
    i32 Stairs = FractalNoise(Map->Seed + 21, X, Y, 24, 2);
    i32 Result = Height > NOISE_PERCENT(48) ? 1 : 0;
    Result += TerraceSteps(Height, NOISE_PERCENT(52), NOISE_PERCENT(8), NOISE_PERCENT(5),
                           3, Stairs > NOISE_PERCENT(52));
    Result = Minimum(Result, 7);
    Result = FlattenNearSpawnAndLandmarks(Map, X, Y, Result);
    return Result;
}

internal terrain_prop
PlaceWildsProp(map_def *Map, i32 X, i32 Y, terrain_kind Ground)
{
    if (InSpawnClearing(X, Y) || Ground != TerrainKind_Grass)
    {
        return TerrainProp_None;
    }
    // NOTE(zoubir): an old paddock, in some stretches of country: a ring of
    // fence with gaps in it, logs lying inside
    i32 Paddock = 0;
    if (FractalNoise(Map->Seed + 31, X, Y, 64, 1) > NOISE_PERCENT(58))
    {
        Paddock = FractalNoise(Map->Seed + 29, X, Y, 14, 1);
    }
    if (Paddock > NOISE_PERCENT(83) && Paddock < NOISE_PERCENT(85))
    {
        return TileRoll(Map, X, Y, 7) < 70 ? TerrainProp_Fence : TerrainProp_None;
    }
    // NOTE(zoubir): groves where the ground is damp, lone trees elsewhere
    i32 Damp = FractalNoise(Map->Seed + 7, X, Y, 24, 2);
    u32 TreeChance = Damp > NOISE_PERCENT(55) ? 18 : 3;
    u32 Roll = TileRoll(Map, X, Y, 1);
    terrain_prop Result = TerrainProp_None;
    if (Roll < TreeChance)
    {
        Result = TerrainProp_Tree;
    }
    else if (Roll < TreeChance + 1)
    {
        Result = TerrainProp_Boulder;
    }
    else if (Paddock >= NOISE_PERCENT(85) ? Roll < TreeChance + 5 :
             (Roll == TreeChance + 1 && TileRoll(Map, X, Y, 5) < 25))
    {
        Result = TerrainProp_Log;
    }
    return Result;
}

internal void
DefineMap_Wilds(map_def *Map)
{
    Map->Name = "Verdant Wilds";
    Map->Kind = MapKind_Infinite;
    Map->Seed = 1907;
    Map->Generate = GenerateWilds;
    Map->PlaceProp = PlaceWildsProp;
    Map->GenerateElevation = GenerateWildsElevation;
    Map->Outside = TerrainKind_Grass;
    Map->SpawnCount = 1;
    Map->MonsterWeight[MonsterKind_Spider] = 2.5f;
    Map->MonsterWeight[MonsterKind_Toad] = 2.5f;
    Map->MonsterWeight[MonsterKind_Slime] = 2.f;
    Map->MonsterWeight[MonsterKind_Bat] = 1.5f;
    Map->MonsterWeight[MonsterKind_Imp] = 0.f;
    Map->MonsterWeight[MonsterKind_Warlord] = 0.f;
    Map->MonsterWeight[MonsterKind_Lurker] = 0.5f;
}

#endif
