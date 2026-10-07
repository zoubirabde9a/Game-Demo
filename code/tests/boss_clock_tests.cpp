/* Boss clock tests (sim/dungeon/boss_clock.cpp), included by
   dungeon_tests.cpp: a boss's enrage timer runs out into harder hits,
   the hits go back to normal once the fight is over, and an add left
   alive past its time merges into the boss and heals it. */

// NOTE(zoubir): the party walks into Room with the rooms before cleared;
// returns the boss the fight spawned
internal world_entity *
StartBossRoom(crypt_world *Crypt, u32 Room)
{
    app_state *AppState = Crypt->AppState;
    dungeon_run *Run = AppState->Dungeon;
    for(u32 Before = 1; Before < Room; Before++)
    {
        Run->RoomStates[Before] = RoomState_Cleared;
    }
    MovePlayerTo(AppState, &AppState->World, &Crypt->Arena, AppState->Players[0].Entity,
                 Run->RoomEntry[Room]);
    TickCrypt(Crypt, 1);
    world_entity *Result = FightBoss(&AppState->World, Run);
    return Result;
}

internal void
TestBossClockEnragesAndHitsHarder()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    dungeon_run *Run = AppState->Dungeon;
    world_entity *Player = AppState->Players[0].Entity;
    world_entity *Boss = StartBossRoom(&Crypt, 3);
    Check(Boss && Boss->MonsterKind == MonsterKind_Gravecaller);
    // NOTE(zoubir): the boss is counted once, not once listed and once
    // for standing in the room
    Check(CountLiveFoes(&AppState->World, Run) == 1);
    Check(Run->Clock.Stage == BossClock_Running);
    // NOTE(zoubir): one player has less boss health to chew through, but
    // also a third of the damage, so it gets longer than three
    Check(Run->Clock.Limit > GetBossClockLimit(MonsterKind_Gravecaller));
    Check(Run->Clock.ShownSecondsLeft > 0);
    float Calm = DungeonScaleDamage(AppState, Player, Boss, 10.f);

    Run->Clock.StartSeconds -= Run->Clock.Limit - 10.f;
    TickCrypt(&Crypt, 1);
    Check(Run->Clock.Stage == BossClock_Warned);
    Check(Run->Clock.ShownSecondsLeft <= 10);

    Run->Clock.StartSeconds -= 11.f;
    TickCrypt(&Crypt, 1);
    Check(Run->Clock.Stage == BossClock_Enraged);
    Check(Boss->Phase == 1);
    float Enraged = DungeonScaleDamage(AppState, Player, Boss, 10.f);
    Check(Enraged > 1.45f * Calm && Enraged < 1.55f * Calm);

    // NOTE(zoubir): it grows the longer the party stays, and Doom hits
    // the party wherever it stands in the room
    float HpBeforeDoom = Player->Hp;
    Run->Clock.StartSeconds -= 2.f * BOSS_ENRAGE_RAMP_SECONDS;
    TickCrypt(&Crypt, 1);
    Check(Run->Clock.DamageScale > BOSS_ENRAGE_DAMAGE + BOSS_ENRAGE_RAMP);
    Check(Run->Clock.DoomPulses >= 5);
    Check(Player->Hp < HpBeforeDoom - BOSS_DOOM_SHARE * Player->MaxHp);

    // NOTE(zoubir): the boss dies, the fight ends, hits are plain again
    KillRoomMonsters(&Crypt, 3);
    TickCrypt(&Crypt, 2);
    Check(Run->FightingRoom == 0);
    Check(BossClockDamageScale(&Run->Clock, Run->FightingRoom, Run->BossSerial) == 1.f);
    DestroyCryptWorld(&Crypt);
}

internal void
TestBossAddsMergeAndHeal()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    dungeon_run *Run = AppState->Dungeon;
    world_entity *Boss = StartBossRoom(&Crypt, 5);
    Check(Boss && Boss->MonsterKind == MonsterKind_BroodQueen);
    Boss->Hp = 0.65f * Boss->MaxHp;
    TickCrypt(&Crypt, 1);
    Check(Run->Clock.AddCount == 2);
    Check(Run->FoeCount == 3);

    // NOTE(zoubir): one spider is killed in time, the other merges
    world *World = &AppState->World;
    world_entity *Killed = FindMonsterBySerial(World, Run->Clock.AddSlots[0],
                                               Run->Clock.AddSerials[0]);
    Check(Killed != 0);
    KillEntity(AppState, World, Killed, 0);
    Run->Clock.AddDeadline[1] = Run->Seconds;
    float Before = Boss->Hp;
    TickCrypt(&Crypt, 1);
    Check(Run->Clock.AddCount == 0);
    Check(Boss->Hp > Before + 0.04f * Boss->MaxHp);
    DestroyCryptWorld(&Crypt);
}

// NOTE(zoubir): the Hollow King binds an armoured champion at 60%; left
// alive past its time it erupts on the party and heals him
internal void
TestHollowChampionErupts()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    world_entity *Player = AppState->Players[0].Entity;
    world_entity *Boss = StartBossRoom(&Crypt, 7);
    Check(Boss && Boss->MonsterKind == MonsterKind_HollowKing);
    // NOTE(zoubir): past the 75% shades, then down to 59%
    Boss->Hp = 0.7f * Boss->MaxHp;
    TickCrypt(&Crypt, 1);
    u32 ShadesTimed = Run->Clock.AddCount;
    Boss->Hp = 0.59f * Boss->MaxHp;
    TickCrypt(&Crypt, 1);
    Check(Run->Clock.AddCount == ShadesTimed + 1);
    u32 Champion = Run->Clock.AddCount - 1;
    world_entity *Add = FindMonsterBySerial(World, Run->Clock.AddSlots[Champion],
                                            Run->Clock.AddSerials[Champion]);
    Check(Add && Add->MonsterKind == MonsterKind_Brute && Add->EliteAffix == MonsterAffix_Armored);
    // NOTE(zoubir): the HUD's count is the next tick's
    TickCrypt(&Crypt, 1);
    Check(Run->Clock.ShownAddBursts && Run->Clock.ShownAddSeconds > 20);

    for(u32 Index = 0; Index < Run->Clock.AddCount; Index++)
    {
        Run->Clock.AddDeadline[Index] = Run->Seconds;
    }
    Player->Hp = Player->MaxHp;
    float BossBefore = Boss->Hp;
    TickCrypt(&Crypt, 1);
    Check(Run->Clock.AddCount == 0);
    Check(Player->Hp < 0.55f * Player->MaxHp);
    Check(Boss->Hp > BossBefore + 0.09f * Boss->MaxHp);
    DestroyCryptWorld(&Crypt);
}

internal void
RunBossClockTests()
{
    TestHollowChampionErupts();
    TestBossClockEnragesAndHitsHarder();
    TestBossAddsMergeAndHeal();
}
