/* Rift tests (dungeon_tests.cpp): the Aurora Rift, the dungeon's fourth
   level. Its rooms match its map, its monsters are tougher, harder
   hitting and faster than the vault's, its bosses stand in their rooms
   on a clock with their adds, and a boss behind its Aurora Pylons takes
   nothing until they are broken (sim/dungeon/boss_wards.cpp). Reaching
   the rift and leaving it are checked with the depths
   (TestClearedCryptGoesDown, depths_tests.cpp); its new ability kinds in
   rift_ability_tests.cpp. */

// NOTE(zoubir): the rift's room map is the size of its layout, every
// gate tile is open ground, seven rooms with a gate between each pair,
// every spawn in the first room, and no gate or room entry on a pit
internal void
TestRiftRoomsMatchTheMap()
{
    map_def *Map = GetMapDef(MapId_Rift);
    Check(Map->Dungeon && Map->MonsterPopulation == 0);
    Check(ArrayCount(RiftRooms) == Map->Height);
    u32 RoomTiles[DUNGEON_MAX_ROOMS + 1] = {};
    u32 GateTiles[DUNGEON_MAX_GATES + 1] = {};
    for(i32 Y = 0; Y < (i32)Map->Height; Y++)
    {
        Check(strlen(RiftRooms[Y]) == Map->Width);
        Check(strlen(RiftLayout[Y]) == Map->Width);
        for(i32 X = 0; X < (i32)Map->Width; X++)
        {
            u32 Room = RoomAtTile(MapId_Rift, X, Y);
            u32 Gate = GateAtTile(MapId_Rift, X, Y);
            terrain_def *Ground = GetTerrainDef(TerrainAt(Map, X, Y));
            if (Gate < DUNGEON_MAX_GATES)
            {
                Check(!Ground->Blocks && !Ground->Hazard);
            }
            RoomTiles[Room]++;
            GateTiles[Gate]++;
        }
    }
    u32 Rooms = CountRooms(MapId_Rift);
    Check(Rooms == 7);
    for(u32 Room = 1; Room <= Rooms; Room++)
    {
        Check(RoomTiles[Room] > 0);
    }
    for(u32 Gate = 0; Gate < DUNGEON_MAX_GATES; Gate++)
    {
        Check((GateTiles[Gate] > 0) == (Gate + 1 < Rooms));
    }
    Check(Map->SpawnCount == MAX_PLAYERS);
    for(u32 Spawn = 0; Spawn < Map->SpawnCount; Spawn++)
    {
        Check(RoomAtTile(MapId_Rift, Map->SpawnX[Spawn], Map->SpawnY[Spawn]) == 1);
    }

    crypt_world Rift = CreateDungeonWorld(MapId_Rift, 1);
    dungeon_run *Run = Rift.AppState->Dungeon;
    world *World = &Rift.AppState->World;
    Check(Run && Run->RoomCount == 7);
    for(u32 Room = 1; Room <= Run->RoomCount; Room++)
    {
        v3 Spots[3] = {Run->RoomEntry[Room], Run->RoomCheckpoint[Room], Run->RoomMiddle[Room]};
        for(u32 Index = 0; Index < ArrayCount(Spots); Index++)
        {
            i32 X = (i32)floorf(Spots[Index].X / (float)World->TileWidth);
            i32 Y = (i32)floorf(Spots[Index].Y / (float)World->TileWidth);
            terrain_def *Ground = GetTerrainDef(TerrainAt(Map, X, Y));
            Check(!Ground->Blocks && !Ground->Hazard);
        }
    }
    DestroyCryptWorld(&Rift);
}

// NOTE(zoubir): each level down is harder than the one before
internal void
TestRiftIsTheHardestLevel()
{
    Check(LevelFoeHealth(MapId_Rift) > LevelFoeHealth(MapId_Vault));
    Check(LevelFoeDamage(MapId_Rift) > LevelFoeDamage(MapId_Vault));
    Check(LevelFoePace(MapId_Rift) > LevelFoePace(MapId_Vault));
    Check(LevelPackScale(MapId_Rift) >= LevelPackScale(MapId_Vault));
    Check(GetDungeonLevel(MapId_Rift)->Number == 4);
}

// NOTE(zoubir): each boss room of the rift starts with its boss in the
// middle, on an enrage clock, and its scripted adds come at their share
internal void
TestRiftBossesStand()
{
    u32 BossRooms[3] = {3, 5, 7};
    u32 BossKinds[3] = {MonsterKind_Grondmaw, MonsterKind_Prism,
                        MonsterKind_Everwinter};
    for(u32 Index = 0; Index < 3; Index++)
    {
        crypt_world Rift = CreateDungeonWorld(MapId_Rift, 1);
        app_state *AppState = Rift.AppState;
        world *World = &AppState->World;
        dungeon_run *Run = AppState->Dungeon;
        u32 Room = BossRooms[Index];
        for(u32 Before = 1; Before < Room; Before++)
        {
            Run->RoomStates[Before] = RoomState_Cleared;
        }
        TickCrypt(&Rift, 1);
        MovePlayerTo(AppState, World, &Rift.Arena, AppState->Players[0].Entity,
                     Run->RoomEntry[Room]);
        TickCrypt(&Rift, 2);
        world_entity *Boss = FightBoss(World, Run);
        Check(Run->FightingRoom == Room && Boss && Boss->MonsterKind == BossKinds[Index]);
        Check(RoomAtPosition(World, Boss->Position.XY) == Room);
        Check(GetMonsterStats(Boss->MonsterKind)->SpawnWeight == 0);
        Check(Run->Clock.Stage == BossClock_Running && Run->Clock.Limit > 60.f);
        u32 Foes = Run->FoeCount;
        Boss->Hp = Boss->MaxHp * 0.55f;
        TickCrypt(&Rift, 1);
        Check(Run->FoeCount > Foes && Run->Clock.AddCount > 0);
        DestroyCryptWorld(&Rift);
    }
}

// NOTE(zoubir): the Prism Warden raises two pylons at 70% and takes
// nothing while one stands; once they are broken it takes hits again
internal void
TestPylonsWardTheBoss()
{
    crypt_world Rift = CreateDungeonWorld(MapId_Rift, 1);
    app_state *AppState = Rift.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    for(u32 Before = 1; Before < 5; Before++)
    {
        Run->RoomStates[Before] = RoomState_Cleared;
    }
    TickCrypt(&Rift, 1);
    world_entity *Player = AppState->Players[0].Entity;
    MovePlayerTo(AppState, World, &Rift.Arena, Player, Run->RoomEntry[5]);
    TickCrypt(&Rift, 2);
    world_entity *Boss = FightBoss(World, Run);
    Check(Boss && Boss->MonsterKind == MonsterKind_Prism);
    Check(!IsWardedBoss(AppState, Boss));

    Boss->Hp = Boss->MaxHp * 0.69f;
    TickCrypt(&Rift, 1);
    u32 Pylons = 0;
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        Pylons += Entity->IsPresent && Entity->Type == EntityType_Monster &&
            Entity->MonsterKind == MonsterKind_AuroraPylon;
    }
    Check(Pylons == 2);
    Check(IsWardedBoss(AppState, Boss));
    float Before = Boss->Hp;
    DamageEntity(AppState, World, Boss, 50.f, Player);
    Check(Boss->Hp == Before);

    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        if (Entity->IsPresent && Entity->Type == EntityType_Monster &&
            Entity->MonsterKind == MonsterKind_AuroraPylon)
        {
            DamageEntity(AppState, World, Entity, 100000.f, Player);
        }
    }
    Check(!IsWardedBoss(AppState, Boss));
    DamageEntity(AppState, World, Boss, 50.f, Player);
    Check(Boss->Hp < Before);
    DestroyCryptWorld(&Rift);
}

// NOTE(zoubir): an Aurora Lance stops at the Prism Sanctum's first pillar
internal void
TestBeamsStopAtPillars()
{
    crypt_world Rift = CreateDungeonWorld(MapId_Rift, 1);
    world *World = &Rift.AppState->World;
    float Tile = (float)World->TileWidth;
    monster_ability *Lance = &GetMonsterDef(MonsterKind_Prism)->Abilities[0];
    Check(Lance->Kind == MonsterAbility_Beam);
    // NOTE(zoubir): from the open floor west of the pillar at tiles 12-13,
    // rows 37-38, straight east into it; and along row 35, which is open
    // all the way to the east wall
    v2 From = V2(8.5f * Tile, 37.5f * Tile);
    float Reach = BeamReach(World, From, V2(1.f, 0.f), Lance);
    Check(Reach > 3.f * Tile && Reach < 4.f * Tile + BEAM_STEP);
    v2 Open = V2(8.5f * Tile, 35.5f * Tile);
    float Long = BeamReach(World, Open, V2(1.f, 0.f), Lance);
    Check(Long > 20.f * Tile);
    DestroyCryptWorld(&Rift);
}

internal void
RunRiftTests()
{
    TestRiftRoomsMatchTheMap();
    TestRiftIsTheHardestLevel();
    TestRiftBossesStand();
    TestPylonsWardTheBoss();
    TestBeamsStopAtPillars();
}
