/* Boss scripts (encounters.cpp): what happens during a boss fight on top
   of the boss's own abilities. One row per event: when the boss of that
   kind first drops to HpShare of its health, Count monsters of AddKind
   climb out at spots round the room, away from the party. They count as
   the encounter's monsters, so the room is not cleared while they live.
   A boss without rows fights with its abilities alone. */

struct boss_event
{
    monster_kind Boss;
    float HpShare;
    monster_kind AddKind;
    u32 Count;
};

global_variable boss_event BossEvents[] =
{
    // NOTE(zoubir): Gravecaller Ossian: a shaman out of the walls twice
    {MonsterKind_Gravecaller, 0.66f, MonsterKind_Shaman, 1},
    {MonsterKind_Gravecaller, 0.33f, MonsterKind_Shaman, 1},
    // NOTE(zoubir): the Brood Queen: two spiders when she is half gone
    {MonsterKind_BroodQueen, 0.5f, MonsterKind_Spider, 2},
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
            }
        }
    }
}
