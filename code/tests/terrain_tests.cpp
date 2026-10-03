/* Terrain tests: integer noise, the map registry, hand-made layouts and
   procedural generators. Included by sim_tests.cpp, which calls
   RunTerrainTests.

   The golden hashes pin the exact ground each procedural map generates.
   They exist so that a compiler (the server builds with g++, players with
   MSVC) or an accidental change that shifts terrain fails here instead of
   desyncing server and clients. When a generator is changed on purpose,
   print the new hash with TERRAIN_PRINT_HASHES and update it. */

#define TERRAIN_PRINT_HASHES 0
#define TERRAIN_GOLDEN_WILDS 0x2D0D5828u
#define TERRAIN_GOLDEN_WASTES 0xB0665B7Cu

internal void
TestFloorDivRoundsDown()
{
    Check(FloorDiv(7, 4) == 1);
    Check(FloorDiv(-1, 4) == -1);
    Check(FloorDiv(-4, 4) == -1);
    Check(FloorDiv(-5, 4) == -2);
    Check(FloorMod(-1, 4) == 3);
    Check(FloorMod(-8, 4) == 0);
}

internal void
TestNoiseStaysInRangeAndIsSmooth()
{
    i32 Min = NOISE_ONE;
    i32 Max = 0;
    for(i32 Y = -200; Y < 200; Y += 7)
    {
        for(i32 X = -200; X < 200; X += 3)
        {
            i32 Value = FractalNoise(42, X, Y, 32, 3);
            Min = Minimum(Min, Value);
            Max = Maximum(Max, Value);
            // NOTE(zoubir): neighbours differ by a little, never a jump
            i32 Next = FractalNoise(42, X + 1, Y, 32, 3);
            i32 Step = Next - Value;
            Check(Step < NOISE_PERCENT(10) && Step > -NOISE_PERCENT(10));
        }
    }
    Check(Min >= 0 && Max <= NOISE_ONE);
    Check(Max - Min > NOISE_PERCENT(40));
}

internal void
TestBoundedLayoutsAreClosedAndValid()
{
    for(u32 MapIndex = 0; MapIndex < MapId_Count; MapIndex++)
    {
        map_def *Map = GetMapDef((map_id)MapIndex);
        Check(Map->Name != 0);
        if (Map->Kind != MapKind_Bounded)
        {
            continue;
        }
        Check(Map->Width > 8 && Map->Height > 8);
        for(u32 Y = 0; Y < Map->Height; Y++)
        {
            Check(strlen(Map->Layout[Y]) == Map->Width);
            for(u32 X = 0; X < Map->Width; X++)
            {
                Check(FindLayoutSymbol(Map->Layout[Y][X]) != 0);
            }
        }
        // NOTE(zoubir): the edge of a bounded map is wall all the way round
        for(u32 X = 0; X < Map->Width; X++)
        {
            Check(GetTerrainDef(TerrainAt(Map, (i32)X, 0))->Blocks);
            Check(GetTerrainDef(TerrainAt(Map, (i32)X, (i32)Map->Height - 1))->Blocks);
        }
        for(u32 Y = 0; Y < Map->Height; Y++)
        {
            Check(GetTerrainDef(TerrainAt(Map, 0, (i32)Y))->Blocks);
            Check(GetTerrainDef(TerrainAt(Map, (i32)Map->Width - 1, (i32)Y))->Blocks);
        }
        Check(GetTerrainDef(TerrainAt(Map, -5, 3))->Blocks);
        Check(Map->SpawnCount >= 1);
        for(u32 Spawn = 0; Spawn < Map->SpawnCount; Spawn++)
        {
            terrain_kind Ground = TerrainAt(Map, Map->SpawnX[Spawn], Map->SpawnY[Spawn]);
            Check(!GetTerrainDef(Ground)->Blocks);
        }
    }
}

internal void
TestInfiniteMapsKeepSpawnOpenEverywhere()
{
    for(u32 MapIndex = 0; MapIndex < MapId_Count; MapIndex++)
    {
        map_def *Map = GetMapDef((map_id)MapIndex);
        if (Map->Kind != MapKind_Infinite)
        {
            continue;
        }
        Check(Map->Generate != 0);
        for(i32 Y = -MAP_SPAWN_CLEARING; Y <= MAP_SPAWN_CLEARING; Y++)
        {
            for(i32 X = -MAP_SPAWN_CLEARING; X <= MAP_SPAWN_CLEARING; X++)
            {
                if (InSpawnClearing(X, Y))
                {
                    Check(!GetTerrainDef(TerrainAt(Map, X, Y))->Blocks);
                    Check(PropAt(Map, X, Y) == TerrainProp_None);
                }
            }
        }
        // NOTE(zoubir): far out, in every direction, the ground is mostly
        // open, and somewhere out there something is in the way
        i32 Far = 2000;
        u32 BlockedAnywhere = 0;
        i32 Corners[4][2] = {{Far, Far}, {-Far, Far}, {Far, -Far}, {-Far, -Far}};
        for(u32 Corner = 0; Corner < 4; Corner++)
        {
            u32 Open = 0;
            u32 Blocked = 0;
            for(i32 Y = 0; Y < 64; Y++)
            {
                for(i32 X = 0; X < 64; X++)
                {
                    terrain_kind Ground = TerrainAt(Map, Corners[Corner][0] + X,
                                                    Corners[Corner][1] + Y);
                    if (GetTerrainDef(Ground)->Blocks)
                    {
                        Blocked++;
                    }
                    else
                    {
                        Open++;
                    }
                }
            }
            Check(Open > 64 * 64 / 2);
            BlockedAnywhere += Blocked;
        }
        Check(BlockedAnywhere > 0);
        // NOTE(zoubir): the same tile asked twice gives the same answer
        Check(TerrainAt(Map, -777, 1234) == TerrainAt(Map, -777, 1234));
        Check(PropAt(Map, -777, 1234) == PropAt(Map, -777, 1234));
    }
}

internal void
TestEveryTerrainKindIsUsed()
{
    bool32 Seen[TerrainKind_Count] = {};
    for(u32 MapIndex = 0; MapIndex < MapId_Count; MapIndex++)
    {
        map_def *Map = GetMapDef((map_id)MapIndex);
        i32 MinX = Map->Kind == MapKind_Bounded ? 0 : -150;
        i32 MinY = Map->Kind == MapKind_Bounded ? 0 : -150;
        i32 Size = Map->Kind == MapKind_Bounded ?
            (i32)Maximum(Map->Width, Map->Height) : 300;
        for(i32 Y = MinY; Y < MinY + Size; Y++)
        {
            for(i32 X = MinX; X < MinX + Size; X++)
            {
                Seen[TerrainAt(Map, X, Y)] = true;
            }
        }
    }
    for(u32 Kind = 0; Kind < TerrainKind_Count; Kind++)
    {
        Check(Seen[Kind]);
        if (!Seen[Kind])
        {
            printf("  terrain kind never placed: %s\n", GetTerrainDef((terrain_kind)Kind)->Name);
        }
    }
}

internal void
TestProceduralTerrainMatchesGoldenHashes()
{
    u32 WildsHash = HashTerrainRegion(GetMapDef(MapId_Wilds), -96, -64, 128);
    u32 WastesHash = HashTerrainRegion(GetMapDef(MapId_Wastes), -96, -64, 128);
#if TERRAIN_PRINT_HASHES
    printf("  wilds 0x%08X wastes 0x%08X\n", WildsHash, WastesHash);
#endif
    Check(WildsHash == TERRAIN_GOLDEN_WILDS);
    Check(WastesHash == TERRAIN_GOLDEN_WASTES);
}

internal void
TestFindMapByName()
{
    Check(FindMapByName("keep", MapId_Arena) == MapId_Keep);
    Check(FindMapByName("Frostbite", MapId_Arena) == MapId_Keep);
    Check(FindMapByName("ASHEN WASTES", MapId_Arena) == MapId_Wastes);
    Check(FindMapByName("wilds", MapId_Arena) == MapId_Wilds);
    Check(FindMapByName("nowhere", MapId_Arena) == MapId_Arena);
    Check(FindMapByName(0, MapId_Keep) == MapId_Keep);
}

// NOTE(zoubir): building a hand-made map puts walls only along open
// ground, and its spawns are free
internal void
TestBuildKeepWorld()
{
    app_state *AppState = (app_state *)calloc(1, sizeof(app_state));
    memory_index Size = Megabytes(64);
    memory_arena Arena;
    memory_arena Constants;
    InitializeArena(&Arena, (memory_index *)calloc(1, Size), Size);
    InitializeArena(&Constants, (memory_index *)calloc(1, Megabytes(1)), Megabytes(1));
    AppState->World.MapId = MapId_Keep;
    InitSimulation(AppState, &Arena, &Constants);
    world *World = &AppState->World;
    map_def *Map = GetMapDef(MapId_Keep);
    Check(World->MapId == MapId_Keep);
    Check(World->NumTilesX == Map->Width && World->NumTilesY == Map->Height);
    for(u32 Slot = 0; Slot < MAX_PLAYERS; Slot++)
    {
        v3 Spawn = PlayerSpawnPosition(World, Slot);
        i32 TileX = (i32)(Spawn.X / World->TileWidth);
        i32 TileY = (i32)(Spawn.Y / World->TileHeight);
        Check(!GetTerrainDef(TerrainAt(Map, TileX, TileY))->Blocks);
    }
    u32 Walls = 0;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        Walls += Entity->IsPresent && Entity->Type == EntityType_StaticObject &&
            Entity->Collision == AppState->WallCollision;
    }
    Check(Walls > 100);
    Check(World->EntityCount < 4096 / 2);
    free(Arena.Base);
    free(Constants.Base);
    free(AppState);
}

internal void
RunTerrainTests()
{
    printf("TestFindMapByName\n");
    TestFindMapByName();
    printf("TestBuildKeepWorld\n");
    TestBuildKeepWorld();
    printf("TestFloorDivRoundsDown\n");
    TestFloorDivRoundsDown();
    printf("TestNoiseStaysInRangeAndIsSmooth\n");
    TestNoiseStaysInRangeAndIsSmooth();
    printf("TestBoundedLayoutsAreClosedAndValid\n");
    TestBoundedLayoutsAreClosedAndValid();
    printf("TestInfiniteMapsKeepSpawnOpenEverywhere\n");
    TestInfiniteMapsKeepSpawnOpenEverywhere();
    printf("TestEveryTerrainKindIsUsed\n");
    TestEveryTerrainKindIsUsed();
    printf("TestProceduralTerrainMatchesGoldenHashes\n");
    TestProceduralTerrainMatchesGoldenHashes();
}
