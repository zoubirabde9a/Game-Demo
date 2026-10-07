/* Boss scripts (encounters.cpp): what happens during a boss fight on top
   of the boss's own abilities. One row per event: when the boss of that
   kind first drops to HpShare of its health, Count monsters of AddKind
   climb out at spots round the room, away from the party. They count as
   the encounter's monsters, so the room is not cleared while they live.
   An add with MergeSeconds that is still alive after them walks back
   into the boss: it dies and heals the boss MergeHeal of its health, so
   the damage players have to turn on the adds, and quickly.
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
};

global_variable boss_event BossEvents[] =
{
    // NOTE(zoubir): Gravecaller Ossian: a shaman out of the walls twice;
    // they mend the dead, so they are killed first anyway
    {MonsterKind_Gravecaller, 0.66f, MonsterKind_Shaman, 1, 0.f, 0.f},
    {MonsterKind_Gravecaller, 0.33f, MonsterKind_Shaman, 1, 0.f, 0.f},
    // NOTE(zoubir): the Brood Queen: her brood crawls out twice and
    // crawls back into her if left alone
    {MonsterKind_BroodQueen, 0.7f, MonsterKind_Spider, 2, 18.f, 0.05f},
    {MonsterKind_BroodQueen, 0.4f, MonsterKind_Spider, 3, 18.f, 0.05f},
    // NOTE(zoubir): the Hollow King: his court of shades rises three
    // times, more each time; a shade that lives 14 s returns to him
    {MonsterKind_HollowKing, 0.75f, MonsterKind_Shade, 2, 14.f, 0.06f},
    {MonsterKind_HollowKing, 0.5f, MonsterKind_Shade, 3, 14.f, 0.06f},
    {MonsterKind_HollowKing, 0.25f, MonsterKind_Shade, 4, 14.f, 0.06f},
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
    UpdateBossClock(AppState, Run, Boss);
    UpdateBossAdds(AppState, World, Run, Boss);
    if (!Boss || Boss->MaxHp <= 0.f)
    {
        return;
    }
    float Share = Boss->Hp / Boss->MaxHp;
    u32 Standing;
    float HealthScale = PartyHealthScale(CountPartyPlayers(AppState, &Standing));
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
                if (Event->MergeSeconds > 0.f)
                {
                    TimeBossAdd(Run, World, Spawned, Event->MergeSeconds,
                                Event->MergeHeal);
                }
            }
        }
    }
}
