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

// NOTE(zoubir): the center of a tile of Kind, preferring one whose 3 x 3
// neighbourhood is all Kind so a moving unit stays on it
internal v3
FindTerrainSpot(map_def *Map, terrain_kind Kind)
{
    for(i32 Y = 1; Y < (i32)Map->Height - 1; Y++)
    {
        for(i32 X = 1; X < (i32)Map->Width - 1; X++)
        {
            bool32 Solid = true;
            for(i32 DY = -1; DY <= 1 && Solid; DY++)
            {
                for(i32 DX = -1; DX <= 1 && Solid; DX++)
                {
                    Solid = TerrainAt(Map, X + DX, Y + DY) == Kind &&
                        PropAt(Map, X + DX, Y + DY) == TerrainProp_None;
                }
            }
            if (Solid)
            {
                return V3((X + 0.5f) * ARENA_TILE_SIZE, (Y + 0.5f) * ARENA_TILE_SIZE, 0.f);
            }
        }
    }
    // NOTE(zoubir): narrow terrain (a two-tile moat): any single tile
    for(i32 Y = 0; Y < (i32)Map->Height; Y++)
    {
        for(i32 X = 0; X < (i32)Map->Width; X++)
        {
            if (TerrainAt(Map, X, Y) == Kind && PropAt(Map, X, Y) == TerrainProp_None)
            {
                return V3((X + 0.5f) * ARENA_TILE_SIZE, (Y + 0.5f) * ARENA_TILE_SIZE, 0.f);
            }
        }
    }
    return V3(-1.f, -1.f, 0.f);
}

internal void
TestGroundRulesApplyToWhoStandsOnIt()
{
    test_world Test = CreateTestWorld();
    Test.World->MapId = MapId_Keep;
    map_def *Map = GetMapDef(MapId_Keep);
    v3 Snow = FindTerrainSpot(Map, TerrainKind_Snow);
    v3 Ice = FindTerrainSpot(Map, TerrainKind_Ice);
    v3 Floor = FindTerrainSpot(Map, TerrainKind_StoneFloor);
    Check(Snow.X > 0.f && Ice.X > 0.f && Floor.X > 0.f);

    world_entity *OnSnow = AddTestPlayer(&Test, Snow);
    world_entity *OnIce = AddTestPlayer(&Test, Ice);
    world_entity *OnFloor = AddTestPlayer(&Test, Floor);
    world_entity *Bat = AddTestMonster(&Test, MonsterKind_Bat, Snow + V3(0.f, 4.f, 0.f));
    Bat->Position.Z = 0.f;
    UpdateTerrainEffects(Test.World);

    Check(GetMoveSpeedScale(OnSnow) == GetTerrainDef(TerrainKind_Snow)->SpeedScale);
    Check(GetMoveSpeedScale(OnFloor) == 1.f);
    Check(GetGroundFriction(OnIce) == GetTerrainDef(TerrainKind_Ice)->Friction);
    Check(GetGroundFriction(OnFloor) == 1.f);
    // NOTE(zoubir): flyers skim over everything
    Check(Bat->GroundSpeedScale == 1.f);
    // NOTE(zoubir): a player in the air ignores the ground under it
    OnSnow->Position.Z = 20.f;
    UpdateTerrainEffects(Test.World);
    Check(GetMoveSpeedScale(OnSnow) == 1.f);
    DestroyTestWorld(&Test);
}

internal void
TestLavaBurnsWhoStandsInIt()
{
    test_world Test = CreateTestWorld();
    Test.World->MapId = MapId_Keep;
    map_def *Map = GetMapDef(MapId_Keep);
    // NOTE(zoubir): the keep's fire pits are single tiles; stand on one
    v3 Pit = V3(-1.f, -1.f, 0.f);
    for(i32 Y = 0; Y < (i32)Map->Height && Pit.X < 0.f; Y++)
    {
        for(i32 X = 0; X < (i32)Map->Width; X++)
        {
            if (TerrainAt(Map, X, Y) == TerrainKind_Lava)
            {
                Pit = V3((X + 0.5f) * ARENA_TILE_SIZE, (Y + 0.5f) * ARENA_TILE_SIZE, 0.f);
                break;
            }
        }
    }
    Check(Pit.X > 0.f);
    world_entity *Player = AddTestPlayer(&Test, Pit);
    StepWorld(&Test, 1);
    UpdateTerrainEffects(Test.World);
    Check(HasStatus(Player, StatusEffect_Burning));
    StepWorld(&Test, 60);
    Check(Player->Hp < 100.f);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): two brutes chase two players across stone floor and snow;
// the one in the snow covers less ground
internal void
TestSnowSlowsMonsters()
{
    test_world Test = CreateTestWorld();
    Test.World->MapId = MapId_Keep;
    map_def *Map = GetMapDef(MapId_Keep);
    v3 Snow = FindTerrainSpot(Map, TerrainKind_Snow);
    v3 Floor = FindTerrainSpot(Map, TerrainKind_StoneFloor);
    world_entity *OnSnow = AddTestMonster(&Test, MonsterKind_Brute, Snow);
    world_entity *OnFloor = AddTestMonster(&Test, MonsterKind_Brute, Floor);
    AddPlayerToSlot(Test.AppState, Test.World, &Test.Arena, 0, Snow + V3(200.f, 0.f, 0.f));
    AddPlayerToSlot(Test.AppState, Test.World, &Test.Arena, 1, Floor + V3(200.f, 0.f, 0.f));
    v3 SnowStart = OnSnow->Position;
    v3 FloorStart = OnFloor->Position;
    for(u32 Frame = 0; Frame < 20; Frame++)
    {
        UpdateTerrainEffects(Test.World);
        StepMonster(&Test, OnSnow, 1);
        StepMonster(&Test, OnFloor, 1);
    }
    float SnowMoved = Length(OnSnow->Position.XY - SnowStart.XY);
    float FloorMoved = Length(OnFloor->Position.XY - FloorStart.XY);
    Check(FloorMoved > 3.f);
    Check(SnowMoved < 0.9f * FloorMoved);
    DestroyTestWorld(&Test);
}

internal void
RunTerrainTests()
{
    printf("TestFindMapByName\n");
    TestFindMapByName();
    printf("TestBuildKeepWorld\n");
    TestBuildKeepWorld();
    printf("TestGroundRulesApplyToWhoStandsOnIt\n");
    TestGroundRulesApplyToWhoStandsOnIt();
    printf("TestLavaBurnsWhoStandsInIt\n");
    TestLavaBurnsWhoStandsInIt();
    printf("TestSnowSlowsMonsters\n");
    TestSnowSlowsMonsters();
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
