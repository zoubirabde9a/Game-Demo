/* Depths tests (dungeon_tests.cpp): the Ember Depths, the dungeon's
   second level. Its rooms match its map, clearing the crypt goes on to
   it with everyone's progress, its monsters are tougher and faster,
   its bosses stand in their rooms on a clock, Sskarra's dive leaves
   magma, and clearing it starts over at the crypt. */

// NOTE(zoubir): the depths' room map is the size of its layout, every
// gate tile is open ground, seven rooms with a gate between each pair,
// and every spawn in the first room
internal void
TestDepthsRoomsMatchTheMap()
{
    map_def *Map = GetMapDef(MapId_Depths);
    Check(Map->Dungeon && Map->MonsterPopulation == 0);
    Check(ArrayCount(DepthsRooms) == Map->Height);
    u32 RoomTiles[DUNGEON_MAX_ROOMS + 1] = {};
    u32 GateTiles[DUNGEON_MAX_GATES + 1] = {};
    for(i32 Y = 0; Y < (i32)Map->Height; Y++)
    {
        Check(strlen(DepthsRooms[Y]) == Map->Width);
        Check(strlen(DepthsLayout[Y]) == Map->Width);
        for(i32 X = 0; X < (i32)Map->Width; X++)
        {
            u32 Room = RoomAtTile(MapId_Depths, X, Y);
            u32 Gate = GateAtTile(MapId_Depths, X, Y);
            if (Gate < DUNGEON_MAX_GATES)
            {
                Check(!GetTerrainDef(TerrainAt(Map, X, Y))->Blocks);
            }
            RoomTiles[Room]++;
            GateTiles[Gate]++;
        }
    }
    u32 Rooms = CountRooms(MapId_Depths);
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
        Check(RoomAtTile(MapId_Depths, Map->SpawnX[Spawn], Map->SpawnY[Spawn]) == 1);
    }
}

// NOTE(zoubir): every dungeon level goes on to another level, and every
// room past the first of each has something in it
internal void
TestLevelsChain()
{
    Check(NextRunMap(MapId_Crypt) == MapId_Depths);
    Check(NextRunMap(MapId_Depths) == MapId_Crypt);
    Check(NextRunMap(MapId_Arena) == MapId_Arena);
    for(u32 Index = 0; Index < ArrayCount(DungeonLevels); Index++)
    {
        dungeon_level *Level = &DungeonLevels[Index];
        Check(GetMapDef((map_id)Level->MapId)->Dungeon);
        Check(GetDungeonLevel(Level->NextMapId) != 0);
        Check(Level->Number == Index + 1);
        u32 Rooms = CountRooms(Level->MapId);
        Check(Level->RoomNameCount == Rooms + 1);
        for(u32 Room = 2; Room <= Rooms; Room++)
        {
            bool32 Found = false;
            for(u32 Row = 0; Row < Level->EncounterCount; Row++)
            {
                Found |= Level->Encounters[Row].Room == Room;
            }
            Check(Found);
        }
    }
}

// NOTE(zoubir): a cleared crypt goes on to the depths: everyone in the
// Cinder Stair with their role, level and talents
internal void
TestClearedCryptGoesDown()
{
    crypt_world Crypt = CreateCryptWorld(2);
    app_state *AppState = Crypt.AppState;
    SetPlayerRole(AppState, &AppState->Players[0], PlayerRole_Healer);
    AppState->Players[1].Level = 6;
    dungeon_run *Run = AppState->Dungeon;
    for(u32 Room = 1; Room <= Run->RoomCount; Room++)
    {
        Run->RoomStates[Room] = RoomState_Cleared;
    }
    TickCrypt(&Crypt, (u32)(21.f * 60.f));
    Check(AppState->World.MapId == MapId_Depths);
    Run = AppState->Dungeon;
    Check(Run && Run->RoomCount == 7 && Run->RoomStates[2] == RoomState_Waiting);
    for(u32 SlotIndex = 0; SlotIndex < 2; SlotIndex++)
    {
        world_entity *Player = AppState->Players[SlotIndex].Entity;
        Check(Player && !IsDeadPlayer(Player));
        Check(RoomAtPosition(&AppState->World, Player->Position.XY) == 1);
    }
    Check(AppState->Players[0].Role == PlayerRole_Healer);
    Check(AppState->Players[1].Level == 6);

    // NOTE(zoubir): and a cleared depths goes back up to the crypt
    for(u32 Room = 1; Room <= Run->RoomCount; Room++)
    {
        Run->RoomStates[Room] = RoomState_Cleared;
    }
    TickCrypt(&Crypt, (u32)(21.f * 60.f));
    Check(AppState->World.MapId == MapId_Crypt);
    Check(AppState->Players[1].Level == 6);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): the depths' packs have the level's extra health over the
// crypt's, and their hits land harder
internal void
TestDepthsFoesAreTougher()
{
    crypt_world Depths = CreateDungeonWorld(MapId_Depths, 1);
    app_state *AppState = Depths.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    TickCrypt(&Depths, 1);
    world_entity *Player = AppState->Players[0].Entity;
    MovePlayerTo(AppState, World, &Depths.Arena, Player, Run->RoomEntry[2]);
    TickCrypt(&Depths, 1);
    Check(Run->FightingRoom == 2 && Run->FoeCount >= 9);
    bool32 SawPlain = false;
    for(u32 Index = 0; Index < Run->FoeCount; Index++)
    {
        world_entity *Foe = &World->Entities[Run->FoeSlots[Index]];
        float Plain = GetMonsterStats(Foe->MonsterKind)->MaxHp * DUNGEON_FOE_HEALTH;
        if (!Foe->EliteAffix)
        {
            SawPlain = true;
            Check(Foe->MaxHp > Plain * 1.34f && Foe->MaxHp < Plain * 1.36f);
        }
    }
    Check(SawPlain);

    // NOTE(zoubir): the same hit from a monster lands 1.1 times as hard
    // here as in the crypt
    world_entity *Foe = &World->Entities[Run->FoeSlots[0]];
    float Before = Player->Hp;
    DamageEntity(AppState, World, Player, 10.f, Foe);
    float Taken = Before - Player->Hp;
    AppState->World.MapId = MapId_Crypt;
    Before = Player->Hp;
    DamageEntity(AppState, World, Player, 10.f, Foe);
    float CryptTaken = Before - Player->Hp;
    AppState->World.MapId = MapId_Depths;
    Check(CryptTaken > 0.f && Taken > CryptTaken * 1.09f && Taken < CryptTaken * 1.11f);
    DestroyCryptWorld(&Depths);
}

// NOTE(zoubir): each boss room of the depths starts with its boss in the
// middle, on an enrage clock, and its scripted adds come at their share
internal void
TestDepthsBossesStand()
{
    u32 BossRooms[3] = {3, 5, 7};
    u32 BossKinds[3] = {MonsterKind_Forgemaster, MonsterKind_CinderWyrm,
                        MonsterKind_EmberTyrant};
    for(u32 Index = 0; Index < 3; Index++)
    {
        crypt_world Depths = CreateDungeonWorld(MapId_Depths, 1);
        app_state *AppState = Depths.AppState;
        world *World = &AppState->World;
        dungeon_run *Run = AppState->Dungeon;
        u32 Room = BossRooms[Index];
        for(u32 Before = 1; Before < Room; Before++)
        {
            Run->RoomStates[Before] = RoomState_Cleared;
        }
        TickCrypt(&Depths, 1);
        MovePlayerTo(AppState, World, &Depths.Arena, AppState->Players[0].Entity,
                     Run->RoomEntry[Room]);
        TickCrypt(&Depths, 2);
        world_entity *Boss = FightBoss(World, Run);
        Check(Run->FightingRoom == Room && Boss && Boss->MonsterKind == BossKinds[Index]);
        Check(GetMonsterStats(Boss->MonsterKind)->SpawnWeight == 0);
        Check(Run->Clock.Stage == BossClock_Running && Run->Clock.Limit > 60.f);
        u32 Foes = Run->FoeCount;
        Boss->Hp = Boss->MaxHp * 0.65f;
        TickCrypt(&Depths, 1);
        Check(Run->FoeCount > Foes && Run->Clock.AddCount > 0);
        DestroyCryptWorld(&Depths);
    }
}

// NOTE(zoubir): every monster a depths encounter spawns plays at the
// level's pace: it moves that much faster than the same monster at the
// crypt's pace, and the crypt plays at its kinds' own speed
internal void
TestDepthsFoesPlayFaster()
{
    float Pace = LevelFoePace(MapId_Depths);
    Check(Pace > 1.f && LevelFoePace(MapId_Crypt) == 1.f && LevelFoePace(MapId_Arena) == 1.f);
    crypt_world Depths = CreateDungeonWorld(MapId_Depths, 1);
    app_state *AppState = Depths.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    TickCrypt(&Depths, 1);
    MovePlayerTo(AppState, World, &Depths.Arena, AppState->Players[0].Entity,
                 Run->RoomEntry[2]);
    TickCrypt(&Depths, 1);
    Check(Run->FightingRoom == 2 && Run->FoeCount > 0);
    for(u32 Index = 0; Index < Run->FoeCount; Index++)
    {
        world_entity *Foe = &World->Entities[Run->FoeSlots[Index]];
        Check(Foe->PaceScale == Pace);
        float Fast = GetMoveSpeedScale(Foe);
        Foe->PaceScale = 0.f;
        float Plain = GetMoveSpeedScale(Foe);
        Foe->PaceScale = Pace;
        Check(Plain > 0.f && Fast > Plain * (Pace - 0.01f) && Fast < Plain * (Pace + 0.01f));
    }
    DestroyCryptWorld(&Depths);
}

// NOTE(zoubir): Sskarra's Magma Dive leaves a pool of burning magma
// where she surfaces
internal void
TestMagmaDiveLeavesMagma()
{
    crypt_world Depths = CreateDungeonWorld(MapId_Depths, 1);
    app_state *AppState = Depths.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    for(u32 Before = 1; Before < 5; Before++)
    {
        Run->RoomStates[Before] = RoomState_Cleared;
    }
    TickCrypt(&Depths, 1);
    MovePlayerTo(AppState, World, &Depths.Arena, AppState->Players[0].Entity,
                 Run->RoomEntry[5]);
    TickCrypt(&Depths, 2);
    world_entity *Boss = FightBoss(World, Run);
    Check(Boss && Boss->MonsterKind == MonsterKind_CinderWyrm);
    monster_def *Def = GetMonsterDef(MonsterKind_CinderWyrm);
    monster_ability *Dive = 0;
    for(u32 Index = 0; Index < Def->AbilityCount; Index++)
    {
        if (Def->Abilities[Index].Kind == MonsterAbility_Burrow)
        {
            Dive = &Def->Abilities[Index];
            Boss->AbilityIndex = Index;
        }
    }
    Check(Dive && Dive->HazardSeconds > 0.f);
    u32 HazardsBefore = 0;
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        HazardsBefore += (World->Entities[Index].IsPresent &&
                          World->Entities[Index].Type == EntityType_MonsterHazard) ? 1 : 0;
    }
    Boss->Burrowed = true;
    Boss->AbilityPoints[0] = Boss->Position.XY;
    EruptFromBurrow(AppState, World, &Depths.Arena, Boss, Dive);
    u32 HazardsAfter = 0;
    world_entity *Pool = 0;
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        if (Entity->IsPresent && Entity->Type == EntityType_MonsterHazard)
        {
            HazardsAfter++;
            Pool = Entity;
        }
    }
    Check(!Boss->Burrowed && HazardsAfter == HazardsBefore + 1);
    Check(Pool && Pool->TimeLeft == Dive->HazardSeconds &&
          Length(Pool->Position.XY - Boss->Position.XY) < 1.f);
    DestroyCryptWorld(&Depths);
}

internal void
RunDepthsTests()
{
    TestDepthsRoomsMatchTheMap();
    TestLevelsChain();
    TestClearedCryptGoesDown();
    TestDepthsFoesAreTougher();
    TestDepthsBossesStand();
    TestDepthsFoesPlayFaster();
    TestMagmaDiveLeavesMagma();
}
