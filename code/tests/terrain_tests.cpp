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
#define TERRAIN_GOLDEN_WILDS_FAR 0xD52C0394u
#define TERRAIN_GOLDEN_WASTES_FAR 0xC52921E1u

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
    // NOTE(zoubir): a second square further out, with landmarks in it
    u32 WildsFar = HashTerrainRegion(GetMapDef(MapId_Wilds), 80, -80, 160);
    u32 WastesFar = HashTerrainRegion(GetMapDef(MapId_Wastes), 80, -80, 160);
#if TERRAIN_PRINT_HASHES
    printf("  wilds 0x%08X wastes 0x%08X far wilds 0x%08X far wastes 0x%08X\n",
           WildsHash, WastesHash, WildsFar, WastesFar);
#endif
    Check(WildsHash == TERRAIN_GOLDEN_WILDS);
    Check(WastesHash == TERRAIN_GOLDEN_WASTES);
    Check(WildsFar == TERRAIN_GOLDEN_WILDS_FAR);
    Check(WastesFar == TERRAIN_GOLDEN_WASTES_FAR);
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

// NOTE(zoubir): every present entity is listed in exactly the chunks its
// box covers, the same check the soak test makes
internal bool32
ChunkListingIsExact(world *World)
{
    u32 Listed[64] = {};
    for(world_chunk *Chunk = World->FirstChunk; Chunk; Chunk = Chunk->NextInWorld)
    {
        for(world_entity_chunk *Block = &Chunk->FirstEntityChunk; Block; Block = Block->Next)
        {
            for(u32 Index = 0; Index < Block->EntityCount; Index++)
            {
                u32 EntityIndex = (u32)(Block->Entities[Index] - World->Entities);
                if (EntityIndex >= ArrayCount(Listed))
                {
                    return false;
                }
                Listed[EntityIndex]++;
            }
        }
    }
    for(u32 Index = 0; Index < World->EntityCount && Index < ArrayCount(Listed); Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        u32 Expected = 0;
        if (Entity->IsPresent && Entity->Collision)
        {
            chunk_range Range = GetEntityChunkRange(World, Entity, Entity->Position);
            Expected = (u32)((Range.MaxX - Range.MinX + 1) * (Range.MaxY - Range.MinY + 1) *
                             (Range.MaxZ - Range.MinZ + 1));
        }
        if (Listed[Index] != Expected)
        {
            return false;
        }
    }
    return true;
}

internal void
TestUnboundedWorldTracksNegativePositions()
{
    test_world Test = CreateTestWorld();
    world *World = Test.World;
    World->Unbounded = true;

    // NOTE(zoubir): a wall far out in negative space stops a unit there
    world_entity *Wall = AddTestEntity(&Test, EntityType_StaticObject,
                                       {-5000.f, -3000.f, 0}, Test.WallVolume);
    world_entity *Walker = AddTestEntity(&Test, EntityType_Player,
                                         {-5100.f, -3000.f, 0}, Test.UnitVolume);
    Walk(&Test, Walker, {1, 0}, 120);
    Check(Walker->Position.X + 15.f <= Wall->Position.X - 16.f + 0.01f);
    Check(ChunkListingIsExact(World));

    // NOTE(zoubir): walking across the origin moves it between chunks on
    // both sides of zero
    world_entity *Crosser = AddTestEntity(&Test, EntityType_Player,
                                          {-40.f, -40.f, 0}, Test.UnitVolume);
    Walk(&Test, Crosser, {0.707f, 0.707f}, 90);
    Check(Crosser->Position.X > 40.f && Crosser->Position.Y > 40.f);
    Check(ChunkListingIsExact(World));

    // NOTE(zoubir): gathering around the walker finds the wall
    rectangle3 Around = RectCenterHalfDims(Walker->Position, V3(200.f, 200.f, 50.f));
    world_entity *Nearby[64];
    u32 Count = GatherEntitiesInBox(World, Around, Nearby, ArrayCount(Nearby));
    bool32 FoundWall = false;
    for(u32 Index = 0; Index < Count; Index++)
    {
        FoundWall |= Nearby[Index] == Wall;
    }
    Check(FoundWall);

    RemoveEntity(World, Wall);
    Check(ChunkListingIsExact(World));
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): true when Entity overlaps any terrain stand-in around it
internal bool32
OverlapsTerrain(world *World, world_entity *Entity)
{
    entity_collision_volume *Total = &Entity->Collision->TotalVolume;
    rectangle3 Box = RectCenterHalfDims(Entity->Position + Total->Offset, Total->HalfDims);
    world_entity *Nearby[128];
    u32 Count = GatherTerrainColliders(World, Box, Nearby, 0, ArrayCount(Nearby));
    for(u32 Index = 0; Index < Count; Index++)
    {
        if (EntityOverlap(Entity, Nearby[Index]))
        {
            return true;
        }
    }
    return false;
}

// NOTE(zoubir): a real game on an infinite map: the player is carried to
// far spots in every direction; at each, the population follows, nothing
// stands in blocking terrain, and the world stays small
internal void
PlayInfiniteMapFarAway(map_id MapId)
{
    app_state *AppState = (app_state *)calloc(1, sizeof(app_state));
    memory_index Size = Megabytes(128);
    memory_arena Arena;
    memory_arena Constants;
    InitializeArena(&Arena, (memory_index *)calloc(1, Size), Size);
    InitializeArena(&Constants, (memory_index *)calloc(1, Megabytes(1)), Megabytes(1));
    AppState->World.MapId = MapId;
    InitSimulation(AppState, &Arena, &Constants);
    world *World = &AppState->World;
    Check(World->Unbounded);
    world_entity *Player = AddPlayerToSlot(AppState, World, &Arena, 0,
                                           PlayerSpawnPosition(World, 0));
    Check(!OverlapsTerrain(World, Player));

    v3 Spots[] =
    {
        {0.f, 0.f, 0.f}, {12000.f, 300.f, 0.f}, {-15000.f, -9000.f, 0.f},
        {800.f, -20000.f, 0.f}, {-30000.f, 25000.f, 0.f},
    };
    for(u32 SpotIndex = 0; SpotIndex < ArrayCount(Spots); SpotIndex++)
    {
        v3 Old = Player->Position;
        Player->Position = FindFreePlayerSpot(AppState, World, Spots[SpotIndex], Player);
        CheckAndChangeEntityChunk(AppState, World, &Arena, Old, Player);
        Check(!OverlapsTerrain(World, Player));
        // NOTE(zoubir): 20 seconds of play; the player wanders in a circle
        for(u32 Frame = 0; Frame < 20 * 60; Frame++)
        {
            float Angle = 2.f * Pi32 * (float)Frame / 600.f;
            AppState->Players[0].Input.Move = V2(Cos(Angle) > 0.f ? 1.f : -1.f,
                                                 Sin(Angle) > 0.f ? 1.f : -1.f);
            AppState->Players[0].Input.Pressed = 0;
            Player->Hp = Player->MaxHp;
            SimulateTick(AppState, &Arena, 1.f / 60.f);
        }
        u32 Near = 0;
        u32 Live = 0;
        for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
        {
            world_entity *Entity = &World->Entities[EntityIndex];
            if (!Entity->IsPresent || Entity->Type != EntityType_Monster)
            {
                continue;
            }
            Live++;
            Near += Length(Entity->Position.XY - Player->Position.XY) < MONSTER_DESPAWN_DISTANCE;
            if (GetMonsterDef(Entity->MonsterKind)->FlyHeight <= 0.f && !Entity->Burrowed)
            {
                Check(!OverlapsTerrain(World, Entity));
            }
        }
        Check(Near >= 3);
        Check(Near == Live);
        Check(!OverlapsTerrain(World, Player));
        Check(World->EntityCount < 600);
    }
    free(Arena.Base);
    free(Constants.Base);
    free(AppState);
}

internal void
TestInfiniteMapsPlayFarFromOrigin()
{
    PlayInfiniteMapFarAway(MapId_Wilds);
    PlayInfiniteMapFarAway(MapId_Wastes);
}

internal void
TestLandmarkLayoutsAreValid()
{
    for(u32 Index = 0; Index < ArrayCount(LandmarkTable); Index++)
    {
        landmark_def *Def = &LandmarkTable[Index];
        u32 Markers = 0;
        for(u32 Y = 0; Y < Def->Height; Y++)
        {
            Check(strlen(Def->Layout[Y]) == Def->Width);
            for(u32 X = 0; X < Def->Width; X++)
            {
                char C = Def->Layout[Y][X];
                Check(C == '?' || FindLayoutSymbol(C) != 0);
                Markers += (C == 'm' || C == 'n');
            }
        }
        Check(Markers == Def->GuardCount);
        Check(Def->GuardCount <= MAX_LANDMARK_GUARDS);
        Check(Def->MapMask != 0);
        // NOTE(zoubir): it must fit its region with the margin
        Check(Def->Width + 8 < LANDMARK_REGION_TILES && Def->Height + 8 < LANDMARK_REGION_TILES);
    }
}

internal void
TestLandmarksStayAwayFromSpawnAndExist()
{
    for(u32 MapIndex = 0; MapIndex < MapId_Count; MapIndex++)
    {
        map_def *Map = GetMapDef((map_id)MapIndex);
        u32 Found = 0;
        for(i32 RY = -5; RY <= 5; RY++)
        {
            for(i32 RX = -5; RX <= 5; RX++)
            {
                landmark_spot Spot = GetRegionLandmark(Map, RX, RY);
                if (!Spot.Present)
                {
                    continue;
                }
                Found++;
                Check(Map->Kind == MapKind_Infinite);
                Check(LandmarkTable[Spot.Landmark].MapMask & (1u << Map->Id));
                // NOTE(zoubir): the same answer every time
                landmark_spot Again = GetRegionLandmark(Map, RX, RY);
                Check(Again.MinX == Spot.MinX && Again.MinY == Spot.MinY &&
                      Again.Landmark == Spot.Landmark);
                Check(!(RX >= -1 && RX <= 0 && RY >= -1 && RY <= 0));
            }
        }
        if (Map->Kind == MapKind_Infinite)
        {
            Check(Found >= 10);
        }
        else
        {
            Check(Found == 0);
        }
    }
}

internal void
TestLandmarkGuardsWakeOnce()
{
    app_state *AppState = (app_state *)calloc(1, sizeof(app_state));
    memory_index Size = Megabytes(64);
    memory_arena Arena;
    memory_arena Constants;
    InitializeArena(&Arena, (memory_index *)calloc(1, Size), Size);
    InitializeArena(&Constants, (memory_index *)calloc(1, Megabytes(1)), Megabytes(1));
    AppState->World.MapId = MapId_Wastes;
    InitSimulation(AppState, &Arena, &Constants);
    world *World = &AppState->World;
    map_def *Map = GetMapDef(MapId_Wastes);
    AppState->Monsters->Target = 0;

    landmark_spot Spot = {};
    for(i32 R = 1; R <= 5 && !Spot.Present; R++)
    {
        Spot = GetRegionLandmark(Map, R, 0);
    }
    Check(Spot.Present);
    landmark_def *Def = &LandmarkTable[Spot.Landmark];
    v3 Center = GetLandmarkCenter(&Spot, (i32)World->TileWidth);

    world_entity *Player = AddPlayerToSlot(AppState, World, &Arena, 0,
                                           Center + V3(0.f, 300.f, 0.f));
    UpdateMonsterPopulation(AppState, World, &Arena, AppState->Monsters, 1.f / 60.f);
    u32 Guards = 0;
    u32 Elites = 0;
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        if (Entity->IsPresent && Entity->Type == EntityType_Monster &&
            Length(Entity->Position.XY - Center.XY) < 400.f)
        {
            Guards++;
            Elites += Entity->EliteAffix != 0;
        }
    }
    Check(Guards >= 1 && Guards <= Def->GuardCount);
    Check(Elites >= 1);

    // NOTE(zoubir): waking again spawns nobody new
    u32 Before = CountLiveMonsters(World);
    UpdateMonsterPopulation(AppState, World, &Arena, AppState->Monsters, 1.f / 60.f);
    Check(CountLiveMonsters(World) == Before);
    Check(Player->IsPresent);
    free(Arena.Base);
    free(Constants.Base);
    free(AppState);
}

// NOTE(zoubir): the pointer aims at the nearest landmark not yet reached,
// and stops aiming at one once the player has stood at it
internal void
TestLandmarkPointerTargetsNearestUnreached()
{
    test_world Test = CreateTestWorld();
    world *World = Test.World;
    World->MapId = MapId_Wilds;
    World->Unbounded = true;
    map_def *Map = GetMapDef(MapId_Wilds);
    landmark_memory Memory = {};

    landmark_spot First;
    v3 FirstCenter;
    Check(FindLandmarkToPoint(World, V3(0.f, 0.f, 0.f), &Memory, &First, &FirstCenter));
    // NOTE(zoubir): no landmark in range of the search is any closer
    float Best = Length(FirstCenter.XY);
    for(i32 RY = -LANDMARK_SEARCH_REGIONS; RY <= LANDMARK_SEARCH_REGIONS; RY++)
    {
        for(i32 RX = -LANDMARK_SEARCH_REGIONS; RX <= LANDMARK_SEARCH_REGIONS; RX++)
        {
            landmark_spot Spot = GetRegionLandmark(Map, RX, RY);
            if (Spot.Present)
            {
                Check(Length(GetLandmarkCenter(&Spot, (i32)World->TileWidth).XY) >= Best - 0.01f);
            }
        }
    }

    // NOTE(zoubir): standing at it marks it reached; the pointer moves on
    landmark_spot Next;
    v3 NextCenter;
    Check(FindLandmarkToPoint(World, FirstCenter, &Memory, &Next, &NextCenter));
    Check(!(Next.RegionX == First.RegionX && Next.RegionY == First.RegionY));
    Check(HasReachedLandmark(&Memory, &First));
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
    printf("TestUnboundedWorldTracksNegativePositions\n");
    TestUnboundedWorldTracksNegativePositions();
    printf("TestInfiniteMapsPlayFarFromOrigin\n");
    TestInfiniteMapsPlayFarFromOrigin();
    printf("TestLandmarkLayoutsAreValid\n");
    TestLandmarkLayoutsAreValid();
    printf("TestLandmarksStayAwayFromSpawnAndExist\n");
    TestLandmarksStayAwayFromSpawnAndExist();
    printf("TestLandmarkGuardsWakeOnce\n");
    TestLandmarkGuardsWakeOnce();
    TestLandmarkPointerTargetsNearestUnreached();
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
