/* Boss scripts (encounters.cpp): what happens during a boss fight on top
   of the boss's own abilities. One row per event: when the boss of that
   kind first drops to HpShare of its health, Count monsters of AddKind
   climb out at spots round the room, away from the party. They count as
   the encounter's monsters, so the room is not cleared while they live.
   An add with MergeSeconds that is still alive after them walks back
   into the boss: it dies and heals the boss MergeHeal of its health, so
   the damage players have to turn on the adds, and quickly. An add with
   BurstShare also erupts as it goes, hitting every player in the room
   for that share of their health: a damage check the party must pass on
   top of the enrage timer. An add with an Affix comes as that elite, so
   it looks the part online as well.
   A boss without rows fights with its abilities alone. Each boss also
   has an enrage timer (boss_clock.cpp). */

#include "boss_clock.cpp"

struct boss_event
{
    monster_kind Boss;
    float HpShare;
    monster_kind AddKind;
    u32 Count;
    float MergeSeconds;
    float MergeHeal;
    float BurstShare;
    u32 Affix;
};

global_variable boss_event BossEvents[] =
{
    // NOTE(zoubir): Gravecaller Ossian: a shaman out of the walls twice;
    // they mend the dead, so they are killed first anyway
    {MonsterKind_Gravecaller, 0.66f, MonsterKind_Shaman, 1, 0.f, 0.f, 0.f, 0},
    {MonsterKind_Gravecaller, 0.33f, MonsterKind_Shaman, 1, 0.f, 0.f, 0.f, 0},
    // NOTE(zoubir): the Brood Queen: her brood crawls out twice and
    // crawls back into her if left alone
    {MonsterKind_BroodQueen, 0.7f, MonsterKind_Spider, 2, 18.f, 0.05f, 0.f, 0},
    {MonsterKind_BroodQueen, 0.4f, MonsterKind_Spider, 3, 18.f, 0.05f, 0.f, 0},
    // NOTE(zoubir): the Hollow King: his court of shades rises three
    // times, more each time; a shade that lives 14 s returns to him
    {MonsterKind_HollowKing, 0.75f, MonsterKind_Shade, 2, 14.f, 0.06f, 0.f, 0},
    {MonsterKind_HollowKing, 0.5f, MonsterKind_Shade, 3, 14.f, 0.06f, 0.f, 0},
    {MonsterKind_HollowKing, 0.25f, MonsterKind_Shade, 4, 14.f, 0.06f, 0.f, 0},
    // NOTE(zoubir): and at 60% and 30% he binds a Hollow Champion, an
    // armoured brute the whole party must burn down in 25 s; if it lives
    // it erupts for half of everyone's health and heals him 10%
    {MonsterKind_HollowKing, 0.6f, MonsterKind_Brute, 1, 25.f, 0.1f, 0.5f, MonsterAffix_Armored},
    {MonsterKind_HollowKing, 0.3f, MonsterKind_Brute, 1, 25.f, 0.1f, 0.5f, MonsterAffix_Armored},
    // NOTE(zoubir): Forgemaster Kragg: an Anvil Guard, an armoured
    // warden, steps off the wall twice; its shell takes hits from the
    // front, so the party has to flank it before it walks back into him
    {MonsterKind_Forgemaster, 0.7f, MonsterKind_Warden, 1, 20.f, 0.08f, 0.f, MonsterAffix_Armored},
    {MonsterKind_Forgemaster, 0.35f, MonsterKind_Warden, 1, 20.f, 0.08f, 0.f, MonsterAffix_Armored},
    // NOTE(zoubir): Sskarra: her young burst out of the floor twice and
    // crawl back into her if left alone
    {MonsterKind_CinderWyrm, 0.66f, MonsterKind_Lurker, 2, 16.f, 0.06f, 0.f, 0},
    {MonsterKind_CinderWyrm, 0.33f, MonsterKind_Lurker, 2, 16.f, 0.06f, 0.f, MonsterAffix_Frenzied},
    // NOTE(zoubir): Vol'karr: imps pour from the braziers three times,
    // more each time, and he binds a Magma Champion, an armoured ravager,
    // twice: kill it in 25 s or it erupts for half of everyone's health
    {MonsterKind_EmberTyrant, 0.75f, MonsterKind_Imp, 2, 14.f, 0.05f, 0.f, 0},
    {MonsterKind_EmberTyrant, 0.5f, MonsterKind_Imp, 3, 14.f, 0.05f, 0.f, 0},
    {MonsterKind_EmberTyrant, 0.25f, MonsterKind_Imp, 4, 14.f, 0.05f, 0.f, 0},
    {MonsterKind_EmberTyrant, 0.6f, MonsterKind_Ravager, 1, 25.f, 0.1f, 0.5f, MonsterAffix_Armored},
    {MonsterKind_EmberTyrant, 0.3f, MonsterKind_Ravager, 1, 25.f, 0.1f, 0.5f, MonsterAffix_Armored},
};

// NOTE(zoubir): the fight's boss, as encounters.cpp spawned it, or 0 once
// it is dead
inline world_entity *
FightBoss(world *World, dungeon_run *Run)
{
    world_entity *Result = Run->BossSerial ?
        FindMonsterBySerial(World, Run->BossSlot, Run->BossSerial) : 0;
    return Result;
}

// NOTE(zoubir): once a tick while a fight lasts
internal void
UpdateBossEvents(app_state *AppState, world *World, memory_arena *Arena,
                 dungeon_run *Run)
{
    world_entity *Boss = FightBoss(World, Run);
    if ((!Boss || Boss->Hp <= 0.f) && Run->BossSerial)
    {
        CrumbleBossAdds(World, Run);
    }
    UpdateBossClock(AppState, Run, Boss);
    UpdateBossAdds(AppState, World, Run, Boss);
    if (!Boss || Boss->MaxHp <= 0.f)
    {
        return;
    }
    float Share = Boss->Hp / Boss->MaxHp;
    u32 Standing;
    float HealthScale = LevelFoeHealth(World->MapId) *
        PartyHealthScale(CountPartyPlayers(AppState, &Standing));
    for(u32 Index = 0; Index < ArrayCount(BossEvents); Index++)
    {
        boss_event *Event = &BossEvents[Index];
        u32 Bit = 1u << Index;
        if (Event->Boss != Boss->MonsterKind || (Run->BossEventsFired & Bit) ||
            Share > Event->HpShare)
        {
            continue;
        }
        Run->BossEventsFired |= Bit;
        encounter_row Row = {Run->FightingRoom, 0, Event->AddKind, 1, 0};
        for(u32 Add = 0; Add < Event->Count; Add++)
        {
            v3 Spot = PickPackSpot(World, Run, Run->FightingRoom);
            world_entity *Spawned = SpawnFoe(AppState, World, Arena, Run, Spot,
                                             &Row, HealthScale);
            if (Spawned)
            {
                EmitBurst(&AppState->Events, SimBurst_Spawn, SIM_NOBODY,
                          Spawned->Position);
                if (Event->Affix)
                {
                    ApplyEliteAffix(Spawned, Event->Affix);
                }
                if (Event->MergeSeconds > 0.f)
                {
                    TimeBossAdd(Run, World, Spawned, Event->MergeSeconds,
                                Event->MergeHeal, Event->BurstShare);
                }
            }
        }
    }
}
