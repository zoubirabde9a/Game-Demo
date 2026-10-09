/* Boss departure tests (sim/dungeon/boss_departures.cpp), included by
   dungeon_tests.cpp: Ommoroth and Nyxara leave their fights at their
   thresholds and come back, out of reach and on a stopped clock while
   gone, with hazards falling on whoever stands still; Varn never leaves,
   and a wipe takes the hazards away. */

// NOTE(zoubir): seconds left on the boss's clock, as the clock counts
inline float
BossClockLeft(dungeon_run *Run)
{
    float Result = Run->Clock.Limit - (Run->Seconds - Run->Clock.StartSeconds);
    return Result;
}

// NOTE(zoubir): living monsters of Kind in the world
internal u32
CountKind(world *World, u32 Kind)
{
    u32 Result = 0;
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        Result += (Entity->IsPresent && Entity->Type == EntityType_Monster &&
                   Entity->MonsterKind == Kind && Entity->Hp > 0.f) ? 1 : 0;
    }
    return Result;
}

internal void
TestOmmorothSinksAndComesBack()
{
    crypt_world Deep = CreateDungeonWorld(MapId_Starless, 1);
    app_state *AppState = Deep.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    world_entity *Boss = EnterStarlessBossRoom(&Deep, 3);
    world_entity *Player = AppState->Players[0].Entity;
    Check(Boss && Boss->MonsterKind == MonsterKind_Ommoroth);
    // NOTE(zoubir): no adds, so only the hazards hurt the player
    Run->BossEventsFired = 0xFFFFFFFF;
    Player->MaxHp = Player->Hp = 100000.f;

    Boss->Hp = 0.7f * Boss->MaxHp;
    TickCrypt(&Deep, 30);
    Check(Run->Departure.BackAt == 0.f && Boss->Position.Z < 1.f);

    Boss->Hp = 0.6f * Boss->MaxHp;
    TickCrypt(&Deep, 1);
    Check(Run->Departure.BackAt > 0.f);
    Check(Boss->Position.Z > OUT_OF_SIGHT_HEIGHT);
    Check(IsOutOfReach(AppState, Boss) && IsDisabled(Boss));
    // NOTE(zoubir): no blow lands, and no spell looks for it
    float Hp = Boss->Hp;
    DamageEntity(AppState, World, Boss, 200.f, Player);
    Check(Boss->Hp == Hp);
    u32 Room = RoomAtPosition(World, Boss->Position.XY);
    Check(NearestFoe(World, Boss->Position.XY, 2000.f, Room, 0, 0) != Boss);

    // NOTE(zoubir): the clock stands still while it is gone
    float Left = BossClockLeft(Run);
    TickCrypt(&Deep, 60);
    Check(Absolute(BossClockLeft(Run) - Left) < 0.05f);
    Check(Boss->Position.Z > OUT_OF_SIGHT_HEIGHT);

    // NOTE(zoubir): by then a maw is coming for the player, who stands
    // still and is bitten
    Check(Run->Departure.HazardCount >= 1);
    Check(CountKind(World, MonsterKind_VoidMaw) >= 1);
    world_entity *Maw = FindMonsterBySerial(World, Run->Departure.HazardSlots[0],
                                            Run->Departure.HazardSerials[0]);
    Check(Maw && Maw->Position.Z > OUT_OF_SIGHT_HEIGHT);
    Check(Length(Maw->Position.XY - Player->Position.XY) < 10.f);
    Check(IsOutOfReach(AppState, Maw));
    float PlayerHp = Player->Hp;
    TickCrypt(&Deep, 90);
    Check(Player->Hp < PlayerHp);

    // NOTE(zoubir): it comes back where it left, and the maws go
    TickCrypt(&Deep, 60 * 7);
    Check(Run->Departure.BackAt == 0.f);
    Check(Boss->Position.Z < 1.f && !IsDisabled(Boss) && !IsOutOfReach(AppState, Boss));
    TickCrypt(&Deep, 60 * 3);
    Check(Run->Departure.HazardCount == 0 && CountKind(World, MonsterKind_VoidMaw) == 0);
    Hp = Boss->Hp;
    DamageEntity(AppState, World, Boss, 10.f, Player);
    Check(Boss->Hp < Hp);

    // NOTE(zoubir): twice, not three times
    Boss->Hp = 0.35f * Boss->MaxHp;
    TickCrypt(&Deep, 1);
    Check(Run->Departure.BackAt > 0.f);
    TickCrypt(&Deep, 60 * 10);
    Check(Run->Departure.BackAt == 0.f);
    Boss->Hp = 0.1f * Boss->MaxHp;
    TickCrypt(&Deep, 30);
    Check(Run->Departure.BackAt == 0.f && Boss->Position.Z < 1.f);
    DestroyCryptWorld(&Deep);
}

// NOTE(zoubir): Nyxara rises with stars falling; Varn stays put
internal void
TestNyxaraRisesAndVarnStays()
{
    crypt_world Deep = CreateDungeonWorld(MapId_Starless, 1);
    dungeon_run *Run = Deep.AppState->Dungeon;
    world_entity *Boss = EnterStarlessBossRoom(&Deep, 7);
    Check(Boss && Boss->MonsterKind == MonsterKind_Nyxara);
    Run->BossEventsFired = 0xFFFFFFFF;
    Deep.AppState->Players[0].Entity->MaxHp = Deep.AppState->Players[0].Entity->Hp = 100000.f;
    Boss->Hp = 0.69f * Boss->MaxHp;
    TickCrypt(&Deep, 60);
    Check(Run->Departure.BackAt > 0.f && Boss->Position.Z > OUT_OF_SIGHT_HEIGHT);
    Check(CountKind(&Deep.AppState->World, MonsterKind_FallingStar) >= 1);
    DestroyCryptWorld(&Deep);

    Deep = CreateDungeonWorld(MapId_Starless, 1);
    Run = Deep.AppState->Dungeon;
    Boss = EnterStarlessBossRoom(&Deep, 5);
    Check(Boss && Boss->MonsterKind == MonsterKind_Varn);
    Run->BossEventsFired = 0xFFFFFFFF;
    for(u32 Step = 1; Step <= 9; Step++)
    {
        Boss->Hp = (1.f - 0.1f * (float)Step) * Boss->MaxHp;
        TickCrypt(&Deep, 5);
        Check(Run->Departure.BackAt == 0.f && Boss->Position.Z < OUT_OF_SIGHT_HEIGHT);
    }
    DestroyCryptWorld(&Deep);
}

// NOTE(zoubir): a party wiped while the boss is gone leaves no hazard
// behind, and the boss of the next try leaves again
internal void
TestWipeWhileAwayClearsHazards()
{
    crypt_world Deep = CreateDungeonWorld(MapId_Starless, 1);
    app_state *AppState = Deep.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    world_entity *Boss = EnterStarlessBossRoom(&Deep, 3);
    Run->BossEventsFired = 0xFFFFFFFF;
    Boss->Hp = 0.6f * Boss->MaxHp;
    TickCrypt(&Deep, 60);
    Check(CountKind(World, MonsterKind_VoidMaw) >= 1);
    KillEntity(AppState, World, AppState->Players[0].Entity, 0);
    TickCrypt(&Deep, 10);
    Check(Run->FightingRoom == 0);
    Check(Run->Departure.BackAt == 0.f && Run->Departure.HazardCount == 0);
    Check(CountKind(World, MonsterKind_VoidMaw) == 0);
    DestroyCryptWorld(&Deep);
}

internal void
RunBossDepartureTests()
{
    TestOmmorothSinksAndComesBack();
    TestNyxaraRisesAndVarnStays();
    TestWipeWhileAwayClearsHazards();
}
