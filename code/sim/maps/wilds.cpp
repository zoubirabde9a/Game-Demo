/* Verdant Wilds: endless green country. Rolling meadows broken by ponds
   and lakes, rocky outcrops on the high ground, muddy hollows where it is
   wet, and worn dirt trails that wander between them. Trees gather in
   groves where the ground is damp. Generated for any tile from the seed. */
#if defined(MAP_NAME_PASS)
MAP(Wilds)
#else

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

    terrain_kind Result = TerrainKind_Grass;
    if (Height < NOISE_PERCENT(33))
    {
        Result = TerrainKind_DeepWater;
    }
    else if (Height < NOISE_PERCENT(37))
    {
        Result = TerrainKind_ShallowWater;
    }
    else if (Height > NOISE_PERCENT(69))
    {
        Result = TerrainKind_Rock;
    }
    else if (Trail < NOISE_PERCENT(4))
    {
        Result = TerrainKind_Dirt;
    }
    else if (Damp > NOISE_PERCENT(62) && Height < NOISE_PERCENT(50))
    {
        Result = TerrainKind_Mud;
    }
    return Result;
}

internal terrain_prop
PlaceWildsProp(map_def *Map, i32 X, i32 Y, terrain_kind Ground)
{
    if (InSpawnClearing(X, Y) || Ground != TerrainKind_Grass)
    {
        return TerrainProp_None;
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
