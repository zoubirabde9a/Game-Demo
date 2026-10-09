/* Starless tests (dungeon_tests.cpp): the Starless Deep, the dungeon's
   fifth level. Its rooms match its map, its monsters are tougher, harder
   hitting and faster than the rift's, its bosses stand in their rooms on
   a clock with their adds, and a raised mirror takes no blow and turns it
   back on the striker (sim/dungeon/mirror_guard.cpp). Reaching the deep
   and leaving it are checked with the depths (TestClearedCryptGoesDown,
   depths_tests.cpp); its new ability kinds in starless_ability_tests.cpp. */

// NOTE(zoubir): the deep's room map is the size of its layout, every
// gate tile is open ground, seven rooms with a gate between each pair,
// every spawn in the first room, and no gate or room entry on a pit
internal void
TestStarlessRoomsMatchTheMap()
{
    map_def *Map = GetMapDef(MapId_Starless);
    Check(Map->Dungeon && Map->MonsterPopulation == 0);
    Check(ArrayCount(StarlessRooms) == Map->Height);
    u32 RoomTiles[DUNGEON_MAX_ROOMS + 1] = {};
    u32 GateTiles[DUNGEON_MAX_GATES + 1] = {};
    for(i32 Y = 0; Y < (i32)Map->Height; Y++)
    {
        Check(strlen(StarlessRooms[Y]) == Map->Width);
        Check(strlen(StarlessLayout[Y]) == Map->Width);
        for(i32 X = 0; X < (i32)Map->Width; X++)
        {
            u32 Room = RoomAtTile(MapId_Starless, X, Y);
            u32 Gate = GateAtTile(MapId_Starless, X, Y);
            terrain_def *Ground = GetTerrainDef(TerrainAt(Map, X, Y));
            if (Gate < DUNGEON_MAX_GATES)
            {
                Check(!Ground->Blocks && !Ground->Hazard);
            }
            RoomTiles[Room]++;
            GateTiles[Gate]++;
        }
    }
    u32 Rooms = CountRooms(MapId_Starless);
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
        Check(RoomAtTile(MapId_Starless, Map->SpawnX[Spawn], Map->SpawnY[Spawn]) == 1);
    }

    crypt_world Deep = CreateDungeonWorld(MapId_Starless, 1);
    dungeon_run *Run = Deep.AppState->Dungeon;
    world *World = &Deep.AppState->World;
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
    DestroyCryptWorld(&Deep);
}

// NOTE(zoubir): the deepest level is the hardest
internal void
TestStarlessIsTheHardestLevel()
{
    Check(LevelFoeHealth(MapId_Starless) > LevelFoeHealth(MapId_Rift));
    Check(LevelFoeDamage(MapId_Starless) > LevelFoeDamage(MapId_Rift));
    Check(LevelFoePace(MapId_Starless) > LevelFoePace(MapId_Rift));
    Check(LevelPackScale(MapId_Starless) > LevelPackScale(MapId_Rift));
    Check(GetDungeonLevel(MapId_Starless)->Number == 5);
    // NOTE(zoubir): and its bosses outlast the rift's at the same place
    Check(GetMonsterDef(MonsterKind_Ommoroth)->MaxHp >= GetMonsterDef(MonsterKind_Grondmaw)->MaxHp);
    Check(GetMonsterDef(MonsterKind_Nyxara)->MaxHp >= GetMonsterDef(MonsterKind_Everwinter)->MaxHp);
}

// NOTE(zoubir): the party walks into boss room Room of the deep, the
// rooms before it cleared; returns the boss
internal world_entity *
EnterStarlessBossRoom(crypt_world *Deep, u32 Room)
{
    app_state *AppState = Deep->AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    for(u32 Before = 1; Before < Room; Before++)
    {
        Run->RoomStates[Before] = RoomState_Cleared;
    }
    TickCrypt(Deep, 1);
    MovePlayerTo(AppState, World, &Deep->Arena, AppState->Players[0].Entity,
                 Run->RoomEntry[Room]);
    TickCrypt(Deep, 2);
    world_entity *Result = FightBoss(World, Run);
    return Result;
}

// NOTE(zoubir): each boss room of the deep starts with its boss in the
// middle, on an enrage clock, and its scripted adds come at their share
internal void
TestStarlessBossesStand()
{
    u32 BossRooms[3] = {3, 5, 7};
    u32 BossKinds[3] = {MonsterKind_Ommoroth, MonsterKind_Varn, MonsterKind_Nyxara};
    for(u32 Index = 0; Index < 3; Index++)
    {
        crypt_world Deep = CreateDungeonWorld(MapId_Starless, 1);
        app_state *AppState = Deep.AppState;
        world *World = &AppState->World;
        dungeon_run *Run = AppState->Dungeon;
        u32 Room = BossRooms[Index];
        world_entity *Boss = EnterStarlessBossRoom(&Deep, Room);
        Check(Run->FightingRoom == Room && Boss && Boss->MonsterKind == BossKinds[Index]);
        Check(RoomAtPosition(World, Boss->Position.XY) == Room);
        Check(GetMonsterStats(Boss->MonsterKind)->SpawnWeight == 0);
        Check(Run->Clock.Stage == BossClock_Running && Run->Clock.Limit > 60.f);
        u32 Foes = Run->FoeCount;
        Boss->Hp = Boss->MaxHp * 0.55f;
        TickCrypt(&Deep, 1);
        Check(Run->FoeCount > Foes && Run->Clock.AddCount > 0);
        DestroyCryptWorld(&Deep);
    }
}

// NOTE(zoubir): while Varn holds up his mirror a blow on him does nothing
// to him and lands on the player who struck instead, at most the
// mirror's Damage; once it is down he takes hits again
internal void
TestMirrorTurnsBlowsBack()
{
    crypt_world Deep = CreateDungeonWorld(MapId_Starless, 1);
    app_state *AppState = Deep.AppState;
    world *World = &AppState->World;
    world_entity *Boss = EnterStarlessBossRoom(&Deep, 5);
    world_entity *Player = AppState->Players[0].Entity;
    Check(Boss && Boss->MonsterKind == MonsterKind_Varn);
    monster_def *Def = GetMonsterDef(MonsterKind_Varn);
    Check(Def->Abilities[0].Kind == MonsterAbility_Reflect);
    Check(!IsRaisingMirror(Boss));

    Boss->AbilityIndex = 0;
    Boss->AbilityPhase = AbilityPhase_Windup;
    Boss->AbilityTimer = 0.5f;
    Check(IsRaisingMirror(Boss) && !GetRaisedMirror(Boss));
    Boss->AbilityPhase = AbilityPhase_Active;
    Boss->AbilityTimer = 2.f;
    float BossBefore = Boss->Hp;
    float PlayerBefore = Player->Hp;
    DamageEntity(AppState, World, Boss, 20.f, Player);
    Check(Boss->Hp == BossBefore);
    Check(Player->Hp < PlayerBefore && Player->Hp >= PlayerBefore - 20.f);
    // NOTE(zoubir): a huge blow comes back no harder than the cap
    PlayerBefore = Player->Hp;
    DamageEntity(AppState, World, Boss, 500.f, Player);
    Check(Boss->Hp == BossBefore);
    Check(Player->Hp >= PlayerBefore - Def->Abilities[0].Damage - 0.01f);
    // NOTE(zoubir): a blow with nobody behind it is stopped, not turned
    DamageEntity(AppState, World, Boss, 20.f, 0);

    Boss->AbilityPhase = AbilityPhase_Ready;
    DamageEntity(AppState, World, Boss, 20.f, Player);
    Check(Boss->Hp < BossBefore);
    DestroyCryptWorld(&Deep);
}

internal void
RunStarlessTests()
{
    TestStarlessRoomsMatchTheMap();
    TestStarlessIsTheHardestLevel();
    TestStarlessBossesStand();
    TestMirrorTurnsBlowsBack();
}
