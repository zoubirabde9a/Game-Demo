/* Ashen Wastes: an endless burnt plain. Grey ash flats over black basalt,
   lava rivers that wind for miles with scorched basalt banks, and basalt
   cliffs on the high ground. Dead trees and boulders are the only cover.
   Generated for any tile from the seed. Fire monsters rule here. */
#if defined(MAP_NAME_PASS)
MAP(Wastes)
#else

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
    else if (Rockiness > NOISE_PERCENT(60))
    {
        Result = TerrainKind_Basalt;
    }
    return Result;
}

internal terrain_prop
PlaceWastesProp(map_def *Map, i32 X, i32 Y, terrain_kind Ground)
{
    if (InSpawnClearing(X, Y))
    {
        return TerrainProp_None;
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
