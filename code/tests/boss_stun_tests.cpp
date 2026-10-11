/* Boss stun tests (dungeon_tests.cpp): a dungeon's boss resists a chain
   of stuns (BossStunScale, sim/status_effects.cpp), every other unit is
   stunned in full, and a fight marks its boss. */

// NOTE(zoubir): ticks Entity's status clocks Seconds on, a frame at a time
internal void
RunStatusClocks(world_entity *Entity, float Seconds)
{
    for(float Time = 0.f; Time < Seconds; Time += 1.f / 60.f)
    {
        CountDownStatusTimers(Entity, 1.f / 60.f);
    }
}

// NOTE(zoubir): full, half, a quarter, then resisted until
// BOSS_STUN_CHAIN_SECONDS after the last that landed, then full again
internal void
TestBossResistsChainedStuns()
{
    world_entity Boss = {};
    Boss.Type = EntityType_Monster;
    Boss.IsBoss = true;
    float Expected[BOSS_STUN_CHAIN_MOST] = {2.f, 1.f, 0.5f};
    for(u32 Stun = 0; Stun < BOSS_STUN_CHAIN_MOST; Stun++)
    {
        ApplyStatus(&Boss, StatusEffect_Stunned, 2.f);
        Check(Absolute(Boss.StatusTimers[StatusEffect_Stunned] - Expected[Stun]) < 0.001f);
        RunStatusClocks(&Boss, 2.5f);
        Check(!HasStatus(&Boss, StatusEffect_Stunned));
    }
    ApplyStatus(&Boss, StatusEffect_Stunned, 2.f);
    Check(!HasStatus(&Boss, StatusEffect_Stunned) && Boss.StunResisted);
    // NOTE(zoubir): still resisted just before the window after the last
    // that landed runs out; the misses do not hold it open
    RunStatusClocks(&Boss, BOSS_STUN_CHAIN_SECONDS - 2.5f - 0.3f);
    ApplyStatus(&Boss, StatusEffect_Stunned, 2.f);
    Check(!HasStatus(&Boss, StatusEffect_Stunned));
    RunStatusClocks(&Boss, 0.4f);
    ApplyStatus(&Boss, StatusEffect_Stunned, 2.f);
    Check(Absolute(Boss.StatusTimers[StatusEffect_Stunned] - 2.f) < 0.001f);

    // NOTE(zoubir): an ordinary monster takes every stun in full
    world_entity Monster = {};
    Monster.Type = EntityType_Monster;
    for(u32 Stun = 0; Stun < 6; Stun++)
    {
        ApplyStatus(&Monster, StatusEffect_Stunned, 2.f);
        Check(Absolute(Monster.StatusTimers[StatusEffect_Stunned] - 2.f) < 0.001f);
        RunStatusClocks(&Monster, 2.5f);
    }
}

// NOTE(zoubir): the Throne of Dust's boss is marked a boss, its adds not;
// a stun it resists says so
internal void
TestFightMarksItsBoss()
{
    crypt_world Crypt = CreateCryptWorld(1);
    app_state *AppState = Crypt.AppState;
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    world_entity *Player = AppState->Players[0].Entity;
    Run->RoomStates[1] = Run->RoomStates[2] = RoomState_Cleared;
    MovePlayerTo(AppState, World, &Crypt.Arena, Player, Run->RoomEntry[3]);
    TickCrypt(&Crypt, 1);
    Check(Run->FightingRoom == 3 && Run->BossSerial);
    world_entity *Boss = &World->Entities[Run->BossSlot];
    Check(Boss->IsBoss);
    u32 Bosses = 0;
    for(u32 Index = 0; Index < Run->FoeCount; Index++)
    {
        Bosses += World->Entities[Run->FoeSlots[Index]].IsBoss ? 1 : 0;
    }
    Check(Bosses == 1);
    Boss->StunChain = BOSS_STUN_CHAIN_MOST;
    Boss->StunChainSeconds = BOSS_STUN_CHAIN_SECONDS;
    ApplyStatus(Boss, StatusEffect_Stunned, 1.f);
    AppState->Events.Count = 0;
    UpdateStatusEffects(AppState, World, 1.f / 60.f);
    bool32 Said = false;
    for(u32 Index = 0; Index < AppState->Events.Count; Index++)
    {
        Said |= AppState->Events.Events[Index].Type == SimEvent_Burst &&
            AppState->Events.Events[Index].Burst == SimBurst_Resisted;
    }
    Check(Said && !HasStatus(Boss, StatusEffect_Stunned));
    DestroyCryptWorld(&Crypt);
}

internal void
RunBossStunTests()
{
    TestBossResistsChainedStuns();
    TestFightMarksItsBoss();
}
