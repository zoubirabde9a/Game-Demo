/* Dungeon tests (sim/dungeon/): roles change health and damage only in a
   dungeon run, and leave the duel alone. Included by sim_tests.cpp,
   which calls RunDungeonTests. */

internal void
TestRolesDoNothingOutsideADungeon()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *Player = AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    Player->SpawnShield = 0.f;
    float MaxHp = Player->MaxHp;
    SetPlayerRole(AppState, &AppState->Players[0], PlayerRole_Tank);
    Check(Player->MaxHp == MaxHp);
    DamageEntity(AppState, Test.World, Player, 10.f, 0);
    Check(Player->Hp == MaxHp - 10.f);
    DestroyTestWorld(&Test);
}

internal void
TestRolesScaleHealthAndDamage()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    dungeon_run Run = {};
    AppState->Dungeon = &Run;
    world_entity *Tank = AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    world_entity *Healer = AddPlayerToSlot(AppState, Test.World, &Test.Arena, 1, {400, 300, 0});
    Tank->SpawnShield = Healer->SpawnShield = 0.f;
    // NOTE(zoubir): a slot joins as a damage player
    Check(Tank->MaxHp == GetRoleDef(PlayerRole_Damage)->MaxHp);
    SetPlayerRole(AppState, &AppState->Players[0], PlayerRole_Tank);
    SetPlayerRole(AppState, &AppState->Players[1], PlayerRole_Healer);
    Check(Tank->MaxHp == 180.f && Tank->Hp == 180.f);
    Check(Healer->MaxHp == 100.f);

    DamageEntity(AppState, Test.World, Tank, 10.f, 0);
    Check(Tank->Hp == 180.f - 7.f);

    world_entity *Monster = AddTestEntity(&Test, EntityType_Monster,
                                          {500, 300, 0}, Test.UnitVolume);
    Monster->MaxHp = Monster->Hp = 100.f;
    DamageEntity(AppState, Test.World, Monster, 20.f, Healer);
    Check(Monster->Hp == 90.f);
    DamageEntity(AppState, Test.World, Monster, 20.f, Tank);
    Check(Monster->Hp == 76.f);

    AppState->Dungeon = 0;
    DestroyTestWorld(&Test);
}

internal void
TestNoDuelMapIsADungeon()
{
    for(u32 MapIndex = 0; MapIndex < MapId_Count; MapIndex++)
    {
        map_def *Map = GetMapDef((map_id)MapIndex);
        if (Map->Dungeon)
        {
            Check(Map->MonsterPopulation == 0);
        }
    }
}

// NOTE(zoubir): the room layout matches the map: every room and gate
// tile is open ground, the rooms are numbered 1 up with a gate between
// each pair, and the party spawns in the first room
internal void
TestCryptRoomsMatchTheMap()
{
    map_def *Map = GetMapDef(MapId_Crypt);
    Check(Map->Dungeon && Map->MonsterPopulation == 0);
    Check(ArrayCount(CryptRooms) == Map->Height);
    u32 RoomTiles[DUNGEON_MAX_ROOMS + 1] = {};
    u32 GateTiles[DUNGEON_MAX_GATES + 1] = {};
    for(i32 Y = 0; Y < (i32)Map->Height; Y++)
    {
        Check(strlen(CryptRooms[Y]) == Map->Width);
        for(i32 X = 0; X < (i32)Map->Width; X++)
        {
            u32 Room = RoomAtTile(MapId_Crypt, X, Y);
            u32 Gate = GateAtTile(MapId_Crypt, X, Y);
            bool32 Open = !GetTerrainDef(TerrainAt(Map, X, Y))->Blocks;
            char Symbol = CryptRooms[Y][X];
            Check(Symbol == '.' || Room || Gate < DUNGEON_MAX_GATES);
            if (Gate < DUNGEON_MAX_GATES)
            {
                Check(Open);
            }
            RoomTiles[Room]++;
            GateTiles[Gate]++;
        }
    }
    u32 Rooms = CountRooms(MapId_Crypt);
    Check(Rooms == 7);
    for(u32 Room = 1; Room <= Rooms; Room++)
    {
        Check(RoomTiles[Room] > 0);
    }
    for(u32 Gate = 0; Gate < DUNGEON_MAX_GATES; Gate++)
    {
        Check((GateTiles[Gate] > 0) == (Gate + 1 < Rooms));
    }
    for(u32 Spawn = 0; Spawn < Map->SpawnCount; Spawn++)
    {
        Check(RoomAtTile(MapId_Crypt, Map->SpawnX[Spawn], Map->SpawnY[Spawn]) == 1);
    }
    Check(Map->SpawnCount == MAX_PLAYERS);
}

// NOTE(zoubir): the duel's vote never offers the dungeon, and a run
// offers nothing
internal void
TestTheMapVoteLeavesTheDungeonAlone()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->World.MapId = MapId_Arena;
    Check(IsVotableMap(AppState, MapId_Keep));
    Check(!IsVotableMap(AppState, MapId_Crypt));
    dungeon_run Run = {};
    AppState->Dungeon = &Run;
    AppState->World.MapId = MapId_Crypt;
    Check(!IsVotableMap(AppState, MapId_Keep));
    AppState->Dungeon = 0;
    DestroyTestWorld(&Test);
}

struct crypt_world
{
    app_state *AppState;
    memory_arena Arena;
    memory_arena Constants;
};

// NOTE(zoubir): the real Sunken Crypt, built as the game builds it, with
// Players players at their spawns in the Antechamber
internal crypt_world
CreateCryptWorld(u32 Players)
{
    crypt_world Result = {};
    Result.AppState = (app_state *)calloc(1, sizeof(app_state));
    memory_index Size = Megabytes(48);
    InitializeArena(&Result.Arena, (memory_index *)calloc(1, Size), Size);
    InitializeArena(&Result.Constants, (memory_index *)calloc(1, Megabytes(1)), Megabytes(1));
    Result.AppState->World.MapId = MapId_Crypt;
    InitSimulation(Result.AppState, &Result.Arena, &Result.Constants);
    for(u32 SlotIndex = 0; SlotIndex < Players; SlotIndex++)
    {
        world_entity *Player =
            AddPlayerToSlot(Result.AppState, &Result.AppState->World, &Result.Arena,
                            SlotIndex, PlayerSpawnPosition(&Result.AppState->World, SlotIndex));
        Player->SpawnShield = 0.f;
    }
    return Result;
}

internal void
DestroyCryptWorld(crypt_world *Crypt)
{
    free(Crypt->AppState);
    free(Crypt->Arena.Base);
    free(Crypt->Constants.Base);
}

internal void
TickCrypt(crypt_world *Crypt, u32 Ticks)
{
    for(u32 Tick = 0; Tick < Ticks; Tick++)
    {
        SimulateTick(Crypt->AppState, &Crypt->Arena, 1.f / 60.f);
    }
}

inline bool32
IsGateClosed(dungeon_run *Run, u32 Gate)
{
    bool32 Result = Run->GateWalls[Gate][0] != 0;
    return Result;
}

internal void
KillRoomMonsters(crypt_world *Crypt, u32 Room)
{
    world *World = &Crypt->AppState->World;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        if (Entity->IsPresent && Entity->Type == EntityType_Monster &&
            RoomAtPosition(World, Entity->Position.XY) == Room)
        {
            KillEntity(Crypt->AppState, World, Entity, 0);
        }
    }
}

// NOTE(zoubir): the party clears the empty Antechamber at once, walks into
// the Bone Halls and the fight starts behind closed gates; killing every
// monster clears the room and opens the way on
internal void
TestRoomsStartClearAndOpenGates()
{
    crypt_world Crypt = CreateCryptWorld(2);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    Check(Run && Run->RoomCount == 7);
    Check(AppState->Monsters && CountLiveMonsters(World) == 0);
    TickCrypt(&Crypt, 1);
    Check(Run->RoomStates[1] == RoomState_Cleared);
    Check(!IsGateClosed(Run, 0) && IsGateClosed(Run, 1));

    world_entity *A = AppState->Players[0].Entity;
    world_entity *B = AppState->Players[1].Entity;
    MovePlayerTo(AppState, World, &Crypt.Arena, A, Run->RoomEntry[2]);
    TickCrypt(&Crypt, 1);
    Check(Run->FightingRoom == 2 && Run->FoeCount == 10);
    Check(IsGateClosed(Run, 0) && IsGateClosed(Run, 1));
    // NOTE(zoubir): B was pulled in from the Antechamber
    Check(RoomAtPosition(World, B->Position.XY) == 2);
    // NOTE(zoubir): two players face 1.4 times the health
    world_entity *Foe = &World->Entities[Run->FoeSlots[0]];
    Check(Foe->MaxHp > GetMonsterStats(Foe->MonsterKind)->MaxHp * 1.39f);

    KillRoomMonsters(&Crypt, 2);
    TickCrypt(&Crypt, 2);
    Check(Run->RoomStates[2] == RoomState_Cleared && Run->FightingRoom == 0);
    Check(!IsGateClosed(Run, 0) && !IsGateClosed(Run, 1) && IsGateClosed(Run, 2));
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): a player down in a fight stays down while anyone stands;
// when the last one falls the party wipes: the boss is gone, the room
// waits again and everyone stands at its checkpoint, in the room before
internal void
TestDownedWaitAndWipesReset()
{
    crypt_world Crypt = CreateCryptWorld(2);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    Run->RoomStates[1] = Run->RoomStates[2] = RoomState_Cleared;
    world_entity *A = AppState->Players[0].Entity;
    world_entity *B = AppState->Players[1].Entity;
    MovePlayerTo(AppState, World, &Crypt.Arena, A, Run->RoomEntry[3]);
    TickCrypt(&Crypt, 1);
    Check(Run->FightingRoom == 3 && Run->FoeCount == 1);

    KillEntity(AppState, World, A, 0);
    TickCrypt(&Crypt, 5 * 60);
    Check(IsDeadPlayer(A));
    Check(Run->FightingRoom == 3);

    KillEntity(AppState, World, B, 0);
    TickCrypt(&Crypt, 1);
    Check(Run->FightingRoom == 0 && Run->RoomStates[3] == RoomState_Waiting);
    Check(Run->Wipes == 1);
    Check(CountLiveMonsters(World) == 0);
    TickCrypt(&Crypt, 3 * 60);
    Check(!IsDeadPlayer(A) && !IsDeadPlayer(B));
    Check(RoomAtPosition(World, A->Position.XY) == 2);
    Check(RoomAtPosition(World, B->Position.XY) == 2);
    Check(!IsGateClosed(Run, 1));
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): in a run a monster goes for whoever has the most threat
// on it, the tank's damage counting four times; a taunt pulls it to the
// taunter at once. Outside a run it goes for the nearest player
internal void
TestThreatAndTaunt()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    dungeon_run *Run = (dungeon_run *)calloc(1, sizeof(dungeon_run));
    world_entity *Tank = AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {200, 300, 0});
    world_entity *Striker = AddPlayerToSlot(AppState, Test.World, &Test.Arena, 1, {460, 300, 0});
    world_entity *Monster = AddTestEntity(&Test, EntityType_Monster,
                                          {500, 300, 0}, Test.UnitVolume);
    Monster->MaxHp = Monster->Hp = 1000.f;
    Monster->MonsterSerial = 1;
    Check(FindMonsterTarget(AppState, Test.World, Monster, 0) == Striker);

    AppState->Dungeon = Run;
    SetPlayerRole(AppState, &AppState->Players[0], PlayerRole_Tank);
    // NOTE(zoubir): nobody has threat yet: the nearest
    Check(FindMonsterTarget(AppState, Test.World, Monster, 0) == Striker);
    DamageEntity(AppState, Test.World, Monster, 20.f, Striker);
    Check(FindMonsterTarget(AppState, Test.World, Monster, 0) == Striker);
    // NOTE(zoubir): 10 from the tank lands as 7, worth 28 threat to 24
    DamageEntity(AppState, Test.World, Monster, 10.f, Tank);
    float Distance = 0.f;
    Check(FindMonsterTarget(AppState, Test.World, Monster, &Distance) == Tank);
    Check(Distance > 299.f && Distance < 301.f);

    DamageEntity(AppState, Test.World, Monster, 50.f, Striker);
    Check(FindMonsterTarget(AppState, Test.World, Monster, 0) == Striker);
    Check(TauntAround(AppState, &Run->Threat, Tank, 400.f) == 1);
    Check(FindMonsterTarget(AppState, Test.World, Monster, 0) == Tank);
    for(u32 Tick = 0; Tick < (u32)(TAUNT_SECONDS * 60.f) + 5; Tick++)
    {
        UpdateThreat(&Run->Threat, 1.f / 60.f);
    }
    // NOTE(zoubir): the taunt ran out but left the tank ahead
    Check(FindMonsterTarget(AppState, Test.World, Monster, 0) == Tank);
    // NOTE(zoubir): a dead player is never the target
    KillEntity(AppState, Test.World, Tank, 0);
    Check(FindMonsterTarget(AppState, Test.World, Monster, 0) == Striker);

    AppState->Dungeon = 0;
    free(Run);
    DestroyTestWorld(&Test);
}

// NOTE(zoubir): a player that ends up on top of a wall (thrown over it)
// is put back where the party is
internal void
TestStrayPlayersComeBack()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    world_entity *Player = AppState->Players[0].Entity;
    v3 OnWall = TileCenter(World, 40, 0);
    v3 From = Player->Position;
    Player->Position = OnWall;
    Player->GroundZ = 0.f;
    CheckAndChangeEntityChunk(AppState, World, &Crypt.Arena, From, Player);
    TickCrypt(&Crypt, 2);
    Check(RoomAtPosition(World, Player->Position.XY) == 1);
    DestroyCryptWorld(&Crypt);
}

internal void
RunDungeonTests()
{
    TestStrayPlayersComeBack();
    TestThreatAndTaunt();
    TestRoomsStartClearAndOpenGates();
    TestDownedWaitAndWipesReset();
    TestCryptRoomsMatchTheMap();
    TestTheMapVoteLeavesTheDungeonAlone();
    TestRolesDoNothingOutsideADungeon();
    TestRolesScaleHealthAndDamage();
    TestNoDuelMapIsADungeon();
}
