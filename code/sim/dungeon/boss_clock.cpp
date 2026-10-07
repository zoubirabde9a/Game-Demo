/* Boss clock (boss_scripts.cpp): every boss fight is a race against the
   boss's enrage timer, so a party that only survives does not win; it
   has to keep its damage up. The clock starts when the boss spawns and
   starts again after a wipe. BOSS_CLOCK_WARNING seconds before the end
   the HUD turns the clock red. When it runs out the boss goes berserk:
   it moves faster, turns its enraged colour if it had not yet, and every
   hit on a player is BOSS_ENRAGE_DAMAGE harder, growing by
   BOSS_ENRAGE_RAMP every BOSS_ENRAGE_RAMP_SECONDS. On top of that, every
   BOSS_DOOM_SECONDS a Doom pulse hits every player in the room for
   BOSS_DOOM_SHARE of their health times the same growing scale, so even
   a boss whose own blows are few (Gravecaller Ossian lobs shells and
   raises the dead) wipes the party within seconds.

   The limits are for the three players the dungeon is made for, and are
   tuned with tools/dungeon_balance.cpp over several seeds of full runs
   (docs/dungeon-plan.md, "Bosses"): the bots, playing the class kits,
   finish the first boss with a minute to spare, the second with half a
   minute, and the last within seconds of its clock or not at all, so a
   party that loses its damage for long, or lets the adds merge, runs
   out. When the class kits change how hard they hit, the boss health in
   sim/monsters/crypt_*.cpp moves with them rather than these limits, so
   the fights keep their length. Another party size scales the limit
   (BossClockPartyScale). */

#define BOSS_CLOCK_WARNING 30.f
#define BOSS_ENRAGE_DAMAGE 1.5f
#define BOSS_ENRAGE_RAMP 0.25f
#define BOSS_ENRAGE_RAMP_SECONDS 5.f
#define BOSS_ENRAGE_MOST 4.f
#define BOSS_ENRAGE_SPEED 1.25f
#define BOSS_ENRAGE_TINT 0xFF4040FF
#define BOSS_DOOM_SECONDS 2.f
#define BOSS_DOOM_SHARE 0.08f

struct boss_clock_def
{
    monster_kind Boss;
    float Seconds;
};

global_variable boss_clock_def BossClockDefs[] =
{
    {MonsterKind_Gravecaller, 170.f},
    {MonsterKind_BroodQueen, 115.f},
    {MonsterKind_HollowKing, 165.f},
};

// NOTE(zoubir): the party the limits are tuned for
#define BOSS_CLOCK_PARTY 3

// NOTE(zoubir): the share of the extra time a party bigger than the tuned
// one gets: party scaling means each player who joins makes the run
// harder (party_scaling.cpp), and a timer that made up the whole
// difference would undo that
#define BOSS_CLOCK_BIG_PARTY_SHARE 0.5f

// NOTE(zoubir): a party of Players has the boss's health grow by the
// party scaling and its own damage grow about by head count, so the
// limit grows by the one over the other, against the tuned party. A
// smaller party, short of a role, gets all of it (one player 1.43 times
// the time); a bigger one only BOSS_CLOCK_BIG_PARTY_SHARE of what it
// would gain (four players 1.04 times, eight 1.7)
inline float
BossClockPartyScale(u32 Players)
{
    Players = Maximum(Players, 1u);
    float Result = (PartyHealthScale(Players) / PartyHealthScale(BOSS_CLOCK_PARTY)) *
        ((float)BOSS_CLOCK_PARTY / (float)Players);
    if (Players > BOSS_CLOCK_PARTY && Result > 1.f)
    {
        Result = 1.f + BOSS_CLOCK_BIG_PARTY_SHARE * (Result - 1.f);
    }
    return Result;
}

// NOTE(zoubir): how long a boss of Kind gives the party; 0 for none
inline float
GetBossClockLimit(u32 Kind)
{
    float Result = 0.f;
    for(u32 Index = 0; Index < ArrayCount(BossClockDefs); Index++)
    {
        if ((u32)BossClockDefs[Index].Boss == Kind)
        {
            Result = BossClockDefs[Index].Seconds;
        }
    }
    return Result;
}

// NOTE(zoubir): the clock ran out: the boss's own enrage, if its health
// had not brought it on yet, and faster on top
internal void
EnrageBoss(app_state *AppState, world_entity *Boss)
{
    monster_def *Def = GetMonsterDef((monster_kind)Boss->MonsterKind);
    if (!Boss->Phase)
    {
        Boss->Phase = 1;
        Boss->PhaseSpeedScale = Def->EnrageSpeedScale > 0.f ? Def->EnrageSpeedScale : 1.f;
    }
    Boss->PhaseSpeedScale *= BOSS_ENRAGE_SPEED;
    Boss->PhaseFlash = ENRAGE_FLASH_SECONDS;
    Boss->Tint = BOSS_ENRAGE_TINT;
    EmitBurst(&AppState->Events, SimBurst_Spawn, SIM_NOBODY, Boss->Position);
}

// NOTE(zoubir): Share of the health of every player in the fight's room
internal void
HitPartyInRoom(app_state *AppState, dungeon_run *Run, float Share)
{
    world *World = &AppState->World;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        world_entity *Player = LivingPlayerInSlot(AppState, SlotIndex);
        if (Player && RoomAtPosition(World, Player->Position.XY) == Run->FightingRoom)
        {
            EmitBurst(&AppState->Events, SimBurst_Impact, SIM_NOBODY, ChestOf(Player));
            DamageEntity(AppState, World, Player, Share * Player->MaxHp, 0);
        }
    }
}

// NOTE(zoubir): a Doom pulse: every living player in the fight's room
// takes BOSS_DOOM_SHARE of their health, times the enrage's scale. It
// comes from nowhere, so the party's scaling does not grow it again, but
// a ward, Shield Wall and a rally still soften it
internal void
DoomPulse(app_state *AppState, dungeon_run *Run)
{
    HitPartyInRoom(AppState, Run, BOSS_DOOM_SHARE * Run->Clock.DamageScale);
}

// NOTE(zoubir): once a tick while a fight lasts, from UpdateBossEvents;
// Boss is the fight's boss, 0 for none
internal void
UpdateBossClock(app_state *AppState, dungeon_run *Run, world_entity *Boss)
{
    boss_clock *Clock = &Run->Clock;
    float Limit = Boss ? GetBossClockLimit(Boss->MonsterKind) : 0.f;
    if (Limit <= 0.f)
    {
        *Clock = {};
        return;
    }
    if (Clock->BossSerial != Boss->MonsterSerial)
    {
        *Clock = {};
        Clock->BossSerial = Boss->MonsterSerial;
        Clock->Stage = BossClock_Running;
        Clock->StartSeconds = Run->Seconds;
        u32 Standing;
        Clock->Limit = Limit * BossClockPartyScale(CountPartyPlayers(AppState, &Standing));
        Clock->DamageScale = 1.f;
    }
    float Left = Clock->Limit - (Run->Seconds - Clock->StartSeconds);
    if (Left <= 0.f)
    {
        if (Clock->Stage != BossClock_Enraged)
        {
            Clock->Stage = BossClock_Enraged;
            EnrageBoss(AppState, Boss);
        }
        float Steps = (float)(u32)(-Left / BOSS_ENRAGE_RAMP_SECONDS);
        Clock->DamageScale = Minimum(BOSS_ENRAGE_MOST,
                                     BOSS_ENRAGE_DAMAGE + BOSS_ENRAGE_RAMP * Steps);
        u32 Pulses = (u32)(-Left / BOSS_DOOM_SECONDS);
        while (Clock->DoomPulses < Pulses)
        {
            Clock->DoomPulses++;
            DoomPulse(AppState, Run);
        }
    }
    else if (Left <= BOSS_CLOCK_WARNING)
    {
        Clock->Stage = BossClock_Warned;
    }
    Clock->ShownStage = Clock->Stage;
    Clock->ShownSecondsLeft = Left > 0.f ? (u32)(Left + 0.999f) : 0;
}

// NOTE(zoubir): Add merges into the boss Seconds from now unless it dies
// first; an add past BOSS_MAX_TIMED_ADDS is not timed
internal void
TimeBossAdd(dungeon_run *Run, world *World, world_entity *Add, float Seconds, float Heal,
            float Burst)
{
    boss_clock *Clock = &Run->Clock;
    if (Clock->AddCount < BOSS_MAX_TIMED_ADDS)
    {
        u32 Index = Clock->AddCount++;
        Clock->AddSlots[Index] = (u32)(Add - World->Entities);
        Clock->AddSerials[Index] = Add->MonsterSerial;
        Clock->AddDeadline[Index] = Run->Seconds + Seconds;
        Clock->AddHeal[Index] = Heal;
        Clock->AddBurst[Index] = Burst;
    }
}

// NOTE(zoubir): once a tick while a fight lasts: adds whose time is up
// die, heal the boss and maybe erupt; dead ones leave the list
internal void
UpdateBossAdds(app_state *AppState, world *World, dungeon_run *Run, world_entity *Boss)
{
    boss_clock *Clock = &Run->Clock;
    u32 Kept = 0;
    for(u32 Index = 0; Index < Clock->AddCount; Index++)
    {
        world_entity *Add = FindMonsterBySerial(World, Clock->AddSlots[Index],
                                                Clock->AddSerials[Index]);
        if (!Add || Add->Hp <= 0.f)
        {
            continue;
        }
        if (Boss && Boss->Hp > 0.f && Run->Seconds >= Clock->AddDeadline[Index])
        {
            EmitBurst(&AppState->Events, SimBurst_Spawn, SIM_NOBODY, Add->Position);
            EmitBurst(&AppState->Events, SimBurst_Spawn, SIM_NOBODY, Boss->Position);
            Boss->Hp = Minimum(Boss->MaxHp, Boss->Hp + Clock->AddHeal[Index] * Boss->MaxHp);
            DamageEntity(AppState, World, Add, Add->Hp, 0);
            if (Clock->AddBurst[Index] > 0.f)
            {
                HitPartyInRoom(AppState, Run, Clock->AddBurst[Index]);
            }
            continue;
        }
        Clock->AddSlots[Kept] = Clock->AddSlots[Index];
        Clock->AddSerials[Kept] = Clock->AddSerials[Index];
        Clock->AddDeadline[Kept] = Clock->AddDeadline[Index];
        Clock->AddHeal[Kept] = Clock->AddHeal[Index];
        Clock->AddBurst[Kept] = Clock->AddBurst[Index];
        Kept++;
    }
    Clock->AddCount = Kept;
    // NOTE(zoubir): the soonest, for the HUD; one that erupts first
    float Soonest = 0.f;
    Clock->ShownAddBursts = false;
    for(u32 Index = 0; Index < Clock->AddCount; Index++)
    {
        float Left = Maximum(0.f, Clock->AddDeadline[Index] - Run->Seconds);
        bool32 Bursts = Clock->AddBurst[Index] > 0.f;
        if ((Bursts && !Clock->ShownAddBursts) || Soonest == 0.f ||
            (Bursts == Clock->ShownAddBursts && Left < Soonest))
        {
            Soonest = Maximum(Left, 0.001f);
            Clock->ShownAddBursts = Bursts;
        }
    }
    Clock->ShownAddSeconds = Soonest > 0.f ? (u32)(Soonest + 0.999f) : 0;
}
