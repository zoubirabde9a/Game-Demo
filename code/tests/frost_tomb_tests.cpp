/* Frost tomb tests (dungeon_tests.cpp): Vaelith's ice tombs
   (sim/dungeon/frost_tombs.cpp). She marks a player she is not after,
   and never a lone one; when the mark's ring closes everyone in it
   freezes; breaking a tomb frees its player; a tomb left standing
   shatters on them and heals her; and when she dies every mark and tomb
   goes and the room clears. */

// NOTE(zoubir): the party of Players fighting Vaelith on the Everwinter
// Throne, she stunned so only the tombs act, the player in slot 0 the one
// she is after; the first mark due at once
internal crypt_world
VaelithFight(u32 Players)
{
    crypt_world Result = CreateDungeonWorld(MapId_Rift, Players);
    app_state *AppState = Result.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    for(u32 Before = 1; Before < 7; Before++)
    {
        Run->RoomStates[Before] = RoomState_Cleared;
    }
    TickCrypt(&Result, 1);
    MovePlayerTo(AppState, World, &Result.Arena, AppState->Players[0].Entity,
                 Run->RoomEntry[7]);
    TickCrypt(&Result, 2);
    world_entity *Boss = FightBoss(World, Run);
    Check(Boss && Boss->MonsterKind == MonsterKind_Everwinter);
    ApplyStatus(Boss, StatusEffect_Stunned, 1000.f);
    AddThreat(&Run->Threat, World, Boss, 0, 1000.f);
    Run->FrostTombs.NextSeconds = Run->Seconds;
    return Result;
}

// NOTE(zoubir): Slot's player put Offset from the room's middle, at full
// health
internal world_entity *
PlaceInThrone(crypt_world *Fight, u32 Slot, v2 Offset)
{
    app_state *AppState = Fight->AppState;
    dungeon_run *Run = AppState->Dungeon;
    world_entity *Player = AppState->Players[Slot].Entity;
    v3 Spot = Run->RoomMiddle[7];
    Spot.XY += Offset;
    MovePlayerTo(AppState, &AppState->World, &Fight->Arena, Player, Spot);
    Player->Hp = Player->MaxHp;
    return Player;
}

internal u32
CountKind(world *World, monster_kind Kind)
{
    u32 Result = 0;
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        Result += Entity->IsPresent && Entity->Type == EntityType_Monster &&
            Entity->MonsterKind == Kind;
    }
    return Result;
}

// NOTE(zoubir): the ticks a mark takes to close, and a few more
inline u32
FrostMarkTicks()
{
    u32 Result = (u32)(GetMonsterDef(MonsterKind_FrostMark)->Abilities[0].Windup * 60.f) + 6;
    return Result;
}

// NOTE(zoubir): the mark goes on the player Vaelith is not after, nothing
// hurts it, and a party of one is never marked
internal void
TestFrostMarkSkipsHerTarget()
{
    crypt_world Fight = VaelithFight(2);
    app_state *AppState = Fight.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    PlaceInThrone(&Fight, 0, V2(-150.f, 0.f));
    PlaceInThrone(&Fight, 1, V2(150.f, 0.f));
    TickCrypt(&Fight, 1);
    frost_tombs *Tombs = &Run->FrostTombs;
    world_entity *Mark = FindMonsterBySerial(World, Tombs->MarkSlot, Tombs->MarkSerial);
    Check(Mark && Tombs->MarkVictim == 2);
    float Hp = Mark->Hp;
    DamageEntity(AppState, World, Mark, 50.f, AppState->Players[0].Entity);
    Check(Mark->Hp == Hp);
    // NOTE(zoubir): it follows its player
    PlaceInThrone(&Fight, 1, V2(150.f, 60.f));
    TickCrypt(&Fight, 2);
    world_entity *Victim = AppState->Players[1].Entity;
    Check(Length(Mark->Position.XY - Victim->Position.XY) < 8.f);
    DestroyCryptWorld(&Fight);

    crypt_world Alone = VaelithFight(1);
    TickCrypt(&Alone, 60);
    Check(!Alone.AppState->Dungeon->FrostTombs.MarkSerial);
    Check(CountKind(&Alone.AppState->World, MonsterKind_FrostMark) == 0);
    DestroyCryptWorld(&Alone);
}

// NOTE(zoubir): when the ring closes the marked player and the friend
// beside them both freeze, held and out of reach; the one far off does not
internal void
TestFrostRingFreezesEveryoneInIt()
{
    crypt_world Fight = VaelithFight(3);
    app_state *AppState = Fight.AppState;
    world *World = &AppState->World;
    frost_tombs *Tombs = &AppState->Dungeon->FrostTombs;
    PlaceInThrone(&Fight, 0, V2(-160.f, 0.f));
    PlaceInThrone(&Fight, 1, V2(150.f, 0.f));
    PlaceInThrone(&Fight, 2, V2(150.f, 40.f));
    TickCrypt(&Fight, FrostMarkTicks());
    Check(!Tombs->MarkSerial && CountKind(World, MonsterKind_FrostMark) == 0);
    Check(!Tombs->TombSerials[0] && Tombs->TombSerials[1] && Tombs->TombSerials[2]);
    Check(CountKind(World, MonsterKind_IceTomb) == 2);
    for(u32 Slot = 1; Slot < 3; Slot++)
    {
        world_entity *Player = AppState->Players[Slot].Entity;
        world_entity *Tomb = FindMonsterBySerial(World, Tombs->TombSlots[Slot],
                                                 Tombs->TombSerials[Slot]);
        Check(Tomb && IsDisabled(Player) && Player->SpawnShield > 0.f);
        Check(Length(Player->Position.XY - Tomb->Position.XY) < 4.f);
        float Hp = Player->Hp;
        HitPlayerWith(AppState, World, Player, FightBoss(World, AppState->Dungeon), 30.f,
                      V2(0.f, 200.f), 0.f, StatusEffect_None, 0.f, SimBurst_MonsterHit);
        Check(Player->Hp == Hp);
    }
    Check(!IsDisabled(AppState->Players[0].Entity));
    DestroyCryptWorld(&Fight);
}

// NOTE(zoubir): a broken tomb lets its player go at once
internal void
TestBreakingATombFreesThePlayer()
{
    crypt_world Fight = VaelithFight(2);
    app_state *AppState = Fight.AppState;
    world *World = &AppState->World;
    frost_tombs *Tombs = &AppState->Dungeon->FrostTombs;
    PlaceInThrone(&Fight, 0, V2(-160.f, 0.f));
    world_entity *Prisoner = PlaceInThrone(&Fight, 1, V2(150.f, 0.f));
    TickCrypt(&Fight, FrostMarkTicks());
    world_entity *Tomb = FindMonsterBySerial(World, Tombs->TombSlots[1], Tombs->TombSerials[1]);
    Check(Tomb && IsDisabled(Prisoner));
    // NOTE(zoubir): scaled by the level and the party
    Check(Tomb->MaxHp > GetMonsterStats(MonsterKind_IceTomb)->MaxHp);
    DamageEntity(AppState, World, Tomb, 2.f * Tomb->MaxHp, AppState->Players[0].Entity);
    TickCrypt(&Fight, 2);
    Check(!Tombs->TombSerials[1] && CountKind(World, MonsterKind_IceTomb) == 0);
    Check(!IsDisabled(Prisoner) && Prisoner->Hp == Prisoner->MaxHp);
    DestroyCryptWorld(&Fight);
}

// NOTE(zoubir): a tomb left standing shatters on its player and heals her
internal void
TestUnbrokenTombShatters()
{
    crypt_world Fight = VaelithFight(2);
    app_state *AppState = Fight.AppState;
    world *World = &AppState->World;
    frost_tombs *Tombs = &AppState->Dungeon->FrostTombs;
    world_entity *Boss = FightBoss(World, AppState->Dungeon);
    Boss->Hp = 0.9f * Boss->MaxHp;
    PlaceInThrone(&Fight, 0, V2(-160.f, 0.f));
    world_entity *Prisoner = PlaceInThrone(&Fight, 1, V2(150.f, 0.f));
    TickCrypt(&Fight, FrostMarkTicks());
    Check(Tombs->TombSerials[1] && Prisoner->Hp == Prisoner->MaxHp);
    float BossHp = Boss->Hp;
    float Windup = GetMonsterDef(MonsterKind_IceTomb)->Abilities[0].Windup;
    TickCrypt(&Fight, (u32)(Windup * 60.f) + 6);
    Check(!Tombs->TombSerials[1] && CountKind(World, MonsterKind_IceTomb) == 0);
    Check(!IsDisabled(Prisoner) && !IsDeadPlayer(Prisoner));
    Check(Absolute(Prisoner->Hp - (1.f - TOMB_SHATTER_SHARE) * Prisoner->MaxHp) < 0.5f);
    Check(Absolute(Boss->Hp - (BossHp + TOMB_HEAL_SHARE * Boss->MaxHp)) < 0.5f);
    DestroyCryptWorld(&Fight);
}

// NOTE(zoubir): Vaelith dies with a player in a tomb: it goes, they walk
// free and the room clears
internal void
TestVaelithDyingFreesEveryone()
{
    crypt_world Fight = VaelithFight(3);
    app_state *AppState = Fight.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    frost_tombs *Tombs = &Run->FrostTombs;
    PlaceInThrone(&Fight, 0, V2(-160.f, 0.f));
    PlaceInThrone(&Fight, 1, V2(150.f, 0.f));
    PlaceInThrone(&Fight, 2, V2(150.f, 200.f));
    TickCrypt(&Fight, FrostMarkTicks());
    u32 Frozen = Tombs->TombSerials[1] ? 1 : 2;
    Check(Tombs->TombSerials[Frozen] && IsDisabled(AppState->Players[Frozen].Entity));

    KillEntity(AppState, World, FightBoss(World, Run), AppState->Players[0].Entity);
    TickCrypt(&Fight, 4);
    Check(CountKind(World, MonsterKind_IceTomb) == 0 &&
          CountKind(World, MonsterKind_FrostMark) == 0);
    Check(!IsDisabled(AppState->Players[Frozen].Entity));
    Check(Run->RoomStates[7] == RoomState_Cleared);
    DestroyCryptWorld(&Fight);
}

internal void
RunFrostTombTests()
{
    TestFrostMarkSkipsHerTarget();
    TestFrostRingFreezesEveryoneInIt();
    TestBreakingATombFreesThePlayer();
    TestUnbrokenTombShatters();
    TestVaelithDyingFreesEveryone();
}
