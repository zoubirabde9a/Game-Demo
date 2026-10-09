/* Boss departures (encounters.cpp, after boss_scripts.cpp): the Starless
   Deep's first and last bosses leave their fight twice for a few
   seconds, and the party has to keep moving until they come back.
   Ommoroth sinks into the dark under the Maw; Nyxara rises into the sky
   over her throne. One row per departure: when the boss of that kind
   first drops to HpShare of its health it is gone for Seconds.

   While it is away the boss is held BOSS_AWAY_HEIGHT over the spot it
   left from (boss_departure.h): out of reach of every blow and spell,
   bumping into nobody, and not drawn (client/draw_entities.cpp draws a
   monster that high as nothing, and client/dungeon/boss_departure_fx.cpp
   draws where it went instead). It is stunned the whole time, so it
   casts nothing, and its enrage clock (boss_clock.cpp) stops. Every
   DEPARTURE_WAVE_SECONDS a hazard of HazardKind comes down over each
   living player in the room (sim/monsters/starless_departure_hazards.cpp)
   and slams the spot under it after its windup, so a player who keeps
   walking is never hit and one who stands still always is. The boss's
   adds stay and keep fighting. Then the boss drops back where it left,
   in a burst, and the fight goes on.

   Varn stays: his mirrors already make the party stop and think. */

#define DEPARTURE_FIRST_WAVE_SECONDS 0.8f
#define DEPARTURE_WAVE_SECONDS 1.6f
// NOTE(zoubir): a hazard is taken away this long after its slam's
// windup, active and recover time, in case it waited a tick to start
#define DEPARTURE_HAZARD_SPARE_SECONDS 0.3f

struct boss_departure_def
{
    monster_kind Boss;
    float HpShare;
    monster_kind HazardKind;
    float Seconds;
};

// NOTE(zoubir): each between the boss's own add waves (BossEvents), so
// the party is never running from the floor while new adds climb out
global_variable boss_departure_def BossDepartures[] =
{
    // NOTE(zoubir): Ommoroth's adds come at 75%, 50% and 25%
    {MonsterKind_Ommoroth, 0.62f, MonsterKind_VoidMaw, 9.f},
    {MonsterKind_Ommoroth, 0.38f, MonsterKind_VoidMaw, 9.f},
    // NOTE(zoubir): Nyxara's at 80%, 60%, 50%, 40% and 20%
    {MonsterKind_Nyxara, 0.7f, MonsterKind_FallingStar, 9.f},
    {MonsterKind_Nyxara, 0.3f, MonsterKind_FallingStar, 9.f},
};

// NOTE(zoubir): Boss held out of the fight for one more tick: high over
// the floor, still, and stunned so it neither moves nor casts. Gravity
// pulls it down a hair before the next tick puts it back
internal void
HoldBossAway(app_state *AppState, world *World, memory_arena *Arena, world_entity *Boss)
{
    v3 OldPosition = Boss->Position;
    Boss->Position.Z = BOSS_AWAY_HEIGHT;
    Boss->Velocity = {};
    Boss->AbilityPhase = AbilityPhase_Ready;
    Boss->AbilityTimer = 0.f;
    Boss->AbilityPointCount = 0;
    Boss->Burrowed = false;
    ApplyStatus(Boss, StatusEffect_Stunned, 0.25f);
    CheckAndChangeEntityChunk(AppState, World, Arena, OldPosition, Boss);
}

// NOTE(zoubir): the boss drops back onto the spot it left from
internal void
BringBossBack(app_state *AppState, world *World, memory_arena *Arena,
              world_entity *Boss, boss_departure *Departure)
{
    v3 OldPosition = Boss->Position;
    Boss->Position = OnGround(World, V3(Boss->Position.X, Boss->Position.Y, 0.f));
    Boss->Velocity = {};
    Boss->StatusTimers[StatusEffect_Stunned] = 0.f;
    CheckAndChangeEntityChunk(AppState, World, Arena, OldPosition, Boss);
    EmitBurst(&AppState->Events, SimBurst_Spawn, SIM_NOBODY, Boss->Position);
    Departure->BackAt = 0.f;
}

// NOTE(zoubir): a hazard over every living player in the fight's room
internal void
DropDepartureWave(app_state *AppState, world *World, memory_arena *Arena,
                  dungeon_run *Run, boss_departure *Departure)
{
    monster_def *Def = GetMonsterDef((monster_kind)Departure->HazardKind);
    monster_ability *Strike = &Def->Abilities[0];
    float Lasts = Strike->Windup + Strike->Active + Strike->Recover +
        DEPARTURE_HAZARD_SPARE_SECONDS;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        world_entity *Player = LivingPlayerInSlot(AppState, SlotIndex);
        if (!Player || Departure->HazardCount >= MAX_DEPARTURE_HAZARDS ||
            RoomAtPosition(World, Player->Position.XY) != Run->FightingRoom)
        {
            continue;
        }
        v3 Spot = V3(Player->Position.X, Player->Position.Y, Def->FlyHeight);
        world_entity *Hazard = SpawnMonster(AppState, World, Arena, Spot,
                                            (monster_kind)Departure->HazardKind);
        // NOTE(zoubir): it comes down at once, not part way charged
        for(u32 Ability = 0; Ability < MAX_MONSTER_ABILITIES; Ability++)
        {
            Hazard->AbilityCooldowns[Ability] = 0.f;
        }
        u32 Index = Departure->HazardCount++;
        Departure->HazardSlots[Index] = (u32)(Hazard - World->Entities);
        Departure->HazardSerials[Index] = Hazard->MonsterSerial;
        Departure->HazardGone[Index] = Run->Seconds + Lasts;
    }
}

// NOTE(zoubir): hazards whose slam is over are taken away; All takes
// every one, when the fight is over
internal void
UpdateDepartureHazards(world *World, boss_departure *Departure, float Seconds, bool32 All)
{
    u32 Kept = 0;
    for(u32 Index = 0; Index < Departure->HazardCount; Index++)
    {
        world_entity *Hazard = FindMonsterBySerial(World, Departure->HazardSlots[Index],
                                                   Departure->HazardSerials[Index]);
        if (!Hazard)
        {
            continue;
        }
        if (All || Seconds >= Departure->HazardGone[Index])
        {
            RemoveEntity(World, Hazard);
            continue;
        }
        Departure->HazardSlots[Kept] = Departure->HazardSlots[Index];
        Departure->HazardSerials[Kept] = Departure->HazardSerials[Index];
        Departure->HazardGone[Kept] = Departure->HazardGone[Index];
        Kept++;
    }
    Departure->HazardCount = Kept;
}

// NOTE(zoubir): the row of BossDepartures Boss leaves on now, if any:
// the first of its rows it has dropped past and not yet left on
internal boss_departure_def *
DueDeparture(world_entity *Boss, boss_departure *Departure)
{
    float Share = Boss->Hp / Boss->MaxHp;
    u32 Ordinal = 0;
    for(u32 Index = 0; Index < ArrayCount(BossDepartures); Index++)
    {
        boss_departure_def *Row = &BossDepartures[Index];
        if (Row->Boss != Boss->MonsterKind)
        {
            continue;
        }
        u32 Bit = 1u << (Ordinal++ & 31);
        if (!(Departure->Fired & Bit) && Share <= Row->HpShare)
        {
            Departure->Fired |= Bit;
            return Row;
        }
    }
    return 0;
}

// NOTE(zoubir): once a tick from UpdateDungeon, fight or not, after the
// boss's own events and clock
internal void
UpdateBossDepartures(app_state *AppState, world *World, memory_arena *Arena,
                     dungeon_run *Run)
{
    boss_departure *Departure = &Run->Departure;
    world_entity *Boss = Run->FightingRoom ? FightBoss(World, Run) : 0;
    if (Boss && (Boss->Hp <= 0.f || Boss->MaxHp <= 0.f))
    {
        Boss = 0;
    }
    UpdateDepartureHazards(World, Departure, Run->Seconds, !Boss);
    float Elapsed = Run->Seconds - Departure->LastSeconds;
    Departure->LastSeconds = Run->Seconds;
    if (!Boss)
    {
        Departure->BackAt = 0.f;
        return;
    }
    if (Departure->BossSerial != Boss->MonsterSerial)
    {
        Departure->BossSerial = Boss->MonsterSerial;
        Departure->Fired = 0;
        Departure->BackAt = 0.f;
        Elapsed = 0.f;
    }

    if (Departure->BackAt > 0.f)
    {
        // NOTE(zoubir): the boss clock waits for the boss
        Run->Clock.StartSeconds += Elapsed;
        if (Run->Seconds >= Departure->BackAt)
        {
            BringBossBack(AppState, World, Arena, Boss, Departure);
            return;
        }
        HoldBossAway(AppState, World, Arena, Boss);
        // NOTE(zoubir): no wave that would still be falling when it is back
        monster_ability *Strike =
            &GetMonsterDef((monster_kind)Departure->HazardKind)->Abilities[0];
        if (Run->Seconds >= Departure->NextWave &&
            Run->Seconds + Strike->Windup < Departure->BackAt)
        {
            DropDepartureWave(AppState, World, Arena, Run, Departure);
            Departure->NextWave = Run->Seconds + DEPARTURE_WAVE_SECONDS;
        }
        return;
    }

    boss_departure_def *Row = DueDeparture(Boss, Departure);
    if (Row)
    {
        EmitBurst(&AppState->Events, SimBurst_Spawn, SIM_NOBODY, Boss->Position);
        Departure->BackAt = Run->Seconds + Row->Seconds;
        Departure->NextWave = Run->Seconds + DEPARTURE_FIRST_WAVE_SECONDS;
        Departure->HazardKind = Row->HazardKind;
        HoldBossAway(AppState, World, Arena, Boss);
    }
}
