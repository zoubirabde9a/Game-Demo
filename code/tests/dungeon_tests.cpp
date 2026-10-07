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

// NOTE(zoubir): the vote changes the mode: a duel can vote for the
// dungeon and a run for a duel map, never for the map being played
internal void
TestTheMapVoteChangesTheMode()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    AppState->World.MapId = MapId_Arena;
    Check(IsVotableMap(AppState, MapId_Keep));
    Check(IsVotableMap(AppState, MapId_Crypt));
    Check(!IsVotableMap(AppState, MapId_Arena));
    Check(FirstMapOfMode(true) == MapId_Crypt);
    Check(FirstMapOfMode(false) == MapId_Arena);
    dungeon_run Run = {};
    AppState->Dungeon = &Run;
    AppState->World.MapId = MapId_Crypt;
    Check(IsVotableMap(AppState, MapId_Keep));
    Check(!IsVotableMap(AppState, MapId_Crypt));
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
    // NOTE(zoubir): two players face 1.45 times the dungeon's health, 1.12
    // times its damage
    world_entity *Foe = &World->Entities[Run->FoeSlots[0]];
    Check(Foe->MaxHp > GetMonsterStats(Foe->MonsterKind)->MaxHp *
          DUNGEON_FOE_HEALTH * 1.44f);
    Check(Run->PartyDamage == DUNGEON_FOE_DAMAGE * PartyDamageScale(2));
    // NOTE(zoubir): only a boss fight hits harder for the boss
    Check(RunBossDamage(Run) == 1.f);

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

// NOTE(zoubir): one press of Button for the player in SlotIndex, one tick
internal void
PressOnce(crypt_world *Crypt, u32 SlotIndex, u32 Button)
{
    Crypt->AppState->Players[SlotIndex].Input.Pressed = Button;
    TickCrypt(Crypt, 1);
    Crypt->AppState->Players[SlotIndex].Input.Pressed = 0;
}

// NOTE(zoubir): a healer standing over a downed ally for three seconds
// brings them back there; stepping away loses the progress
internal void
TestHealersRevive()
{
    crypt_world Crypt = CreateCryptWorld(3);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    Run->RoomStates[1] = RoomState_Cleared;
    SetPlayerRole(AppState, &AppState->Players[1], PlayerRole_Healer);
    world_entity *Down = AppState->Players[0].Entity;
    world_entity *Healer = AppState->Players[1].Entity;
    world_entity *Other = AppState->Players[2].Entity;
    MovePlayerTo(AppState, World, &Crypt.Arena, Down, Run->RoomEntry[2]);
    TickCrypt(&Crypt, 1);
    Check(Run->FightingRoom == 2);
    // NOTE(zoubir): the room's monsters are taken out of the way; one
    // stays, stunned, so the fight goes on
    for(u32 Index = 1; Index < Run->FoeCount; Index++)
    {
        world_entity *Foe = FindMonsterBySerial(World, Run->FoeSlots[Index], Run->FoeSerials[Index]);
        if (Foe)
        {
            RemoveEntity(World, Foe);
        }
    }
    world_entity *Last = FindMonsterBySerial(World, Run->FoeSlots[0], Run->FoeSerials[0]);
    Check(Last != 0);
    ApplyStatus(Last, StatusEffect_Stunned, 100.f);

    // NOTE(zoubir): the fight pulled the healer in beside the body
    v3 Lying = Down->Position;
    MovePlayerTo(AppState, World, &Crypt.Arena, Healer, Lying + V3(300.f, 0.f, 0.f));
    KillEntity(AppState, World, Down, 0);
    MovePlayerTo(AppState, World, &Crypt.Arena, Other, Lying + V3(0.f, 40.f, 0.f));
    TickCrypt(&Crypt, 4 * 60);
    Check(IsDeadPlayer(Down));

    MovePlayerTo(AppState, World, &Crypt.Arena, Healer, Lying + V3(30.f, 0.f, 0.f));
    TickCrypt(&Crypt, 2 * 60);
    Check(IsDeadPlayer(Down));
    MovePlayerTo(AppState, World, &Crypt.Arena, Healer, Lying + V3(300.f, 0.f, 0.f));
    TickCrypt(&Crypt, 2 * 60);
    Check(IsDeadPlayer(Down));
    Check(AppState->Players[0].ReviveSeconds == 0.f);
    MovePlayerTo(AppState, World, &Crypt.Arena, Healer, Lying + V3(30.f, 0.f, 0.f));
    TickCrypt(&Crypt, (u32)(3.2f * 60.f));
    Check(!IsDeadPlayer(Down));
    Check(Length(Down->Position.XY - Lying.XY) < 1.f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): Gravecaller Ossian waits in the Ossuary; at two thirds
// of its health a Bone Shaman joins the fight, once
internal void
TestBossEventsFireOnce()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    Run->RoomStates[1] = Run->RoomStates[2] = RoomState_Cleared;
    world_entity *Player = AppState->Players[0].Entity;
    MovePlayerTo(AppState, World, &Crypt.Arena, Player, Run->RoomEntry[3]);
    TickCrypt(&Crypt, 1);
    world_entity *Boss = FightBoss(World, Run);
    Check(Boss && Boss->MonsterKind == MonsterKind_Gravecaller);
    Check(Run->FoeCount == 1);
    Check(RunBossDamage(Run) == DUNGEON_BOSS_DAMAGE);
    Boss->Hp = 0.7f * Boss->MaxHp;
    TickCrypt(&Crypt, 1);
    Check(Run->FoeCount == 1);
    Boss->Hp = 0.6f * Boss->MaxHp;
    TickCrypt(&Crypt, 2);
    Check(Run->FoeCount == 2);
    Check(World->Entities[Run->FoeSlots[1]].MonsterKind == MonsterKind_Shaman);
    TickCrypt(&Crypt, 2);
    Check(Run->FoeCount == 2);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): a cleared crypt waits, then a new run starts with
// everyone back in the Antechamber, keeping their role
internal void
TestClearedCryptStartsANewRun()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    SetPlayerRole(AppState, &AppState->Players[0], PlayerRole_Tank);
    dungeon_run *Run = AppState->Dungeon;
    for(u32 Room = 1; Room <= Run->RoomCount; Room++)
    {
        Run->RoomStates[Room] = RoomState_Cleared;
    }
    Run->Wipes = 2;
    TickCrypt(&Crypt, (u32)(10.f * 60.f));
    Check(AppState->Dungeon->Wipes == 2);
    Check(AppState->Dungeon->VictorySeconds > 9.f);
    TickCrypt(&Crypt, (u32)(11.f * 60.f));
    Run = AppState->Dungeon;
    Check(Run && Run->Wipes == 0 && Run->VictorySeconds < 2.f);
    Check(Run->RoomStates[1] == RoomState_Cleared);
    Check(Run->RoomStates[2] == RoomState_Waiting);
    world_entity *Player = AppState->Players[0].Entity;
    Check(Player && !IsDeadPlayer(Player));
    Check(RoomAtPosition(&AppState->World, Player->Position.XY) == 1);
    Check(AppState->Players[0].Role == PlayerRole_Tank && Player->MaxHp == 180.f);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): a blink aimed past a closed gate lands short of it; with
// the gate open it goes through
internal void
TestBlinksStopAtClosedGates()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    TickCrypt(&Crypt, 1);
    world_entity *Player = AppState->Players[0].Entity;
    float Tile = (float)World->TileWidth;
    // NOTE(zoubir): in the corridor west of the first gate (column 18)
    MovePlayerTo(AppState, World, &Crypt.Arena, Player, TileCenter(World, 16, 11));
    v2 Beyond = TileCenter(World, 23, 11).XY;
    Check(!IsGateClosed(Run, 0));
    Check(FindBlinkLanding(AppState, Player, Beyond).X > 20.f * Tile);
    SetGateClosed(AppState, World, &Crypt.Arena, Run, 0, true);
    Check(IsGateClosed(Run, 0));
    Check(FindBlinkLanding(AppState, Player, Beyond).X < 18.f * Tile);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): in a run a player's hit on another does nothing, not even
// a shove; outside one the duel's rules hold
internal void
TestNoFriendlyFireInADungeon()
{
    test_world Test = CreateTestWorld();
    app_state *AppState = Test.AppState;
    world_entity *A = AddPlayerToSlot(AppState, Test.World, &Test.Arena, 0, {300, 300, 0});
    world_entity *B = AddPlayerToSlot(AppState, Test.World, &Test.Arena, 1, {400, 300, 0});
    A->SpawnShield = B->SpawnShield = 0.f;
    dungeon_run *Run = (dungeon_run *)calloc(1, sizeof(dungeon_run));
    AppState->Dungeon = Run;
    float Hp = B->Hp;
    DamageEntity(AppState, Test.World, B, 20.f, A);
    Check(B->Hp == Hp);
    hit Hit = {20.f, 500.f, 0.f, 0.f, 1.f, SimBurst_Count, StatusEffect_None, 0.f};
    Check(!ApplyHit(AppState, Test.World, B, &Hit, V2(1.f, 0.f), A, 0));
    Check(B->Hp == Hp && B->Velocity.X == 0.f);
    // NOTE(zoubir): monsters still hurt players
    world_entity *Monster = AddTestEntity(&Test, EntityType_Monster, {500, 300, 0},
                                          Test.UnitVolume);
    Monster->MaxHp = Monster->Hp = 100.f;
    DamageEntity(AppState, Test.World, B, 10.f, Monster);
    Check(B->Hp < Hp);

    AppState->Dungeon = 0;
    Hp = B->Hp;
    DamageEntity(AppState, Test.World, B, 5.f, A);
    Check(B->Hp == Hp - 5.f);
    free(Run);
    DestroyTestWorld(&Test);
}

#include "dungeon_role_tests.cpp"
#include "boss_clock_tests.cpp"
#include "striker_tests.cpp"
#include "class_kit_tests.cpp"
#include "fight_end_tests.cpp"

internal void
RunDungeonTests()
{
    RunDungeonRoleTests();
    RunBossClockTests();
    RunStrikerTests();
    RunClassKitTests();
    RunFightEndTests();
    TestNoFriendlyFireInADungeon();
    TestBlinksStopAtClosedGates();
    TestClearedCryptStartsANewRun();
    TestBossEventsFireOnce();
    TestHealersRevive();
    TestStrayPlayersComeBack();
    TestThreatAndTaunt();
    TestRoomsStartClearAndOpenGates();
    TestDownedWaitAndWipesReset();
    TestCryptRoomsMatchTheMap();
    TestTheMapVoteChangesTheMode();
    TestRolesDoNothingOutsideADungeon();
    TestRolesScaleHealthAndDamage();
    TestNoDuelMapIsADungeon();
}
