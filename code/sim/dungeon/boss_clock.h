/* Boss clock (boss_clock.cpp), the part dungeon.cpp needs before the
   rest: the state a dungeon_run keeps for the boss being fought, and the
   scale every hit on a player takes once the clock has run out
   (DungeonScaleDamage): the boss's own blows, its shots and its adds',
   so a party that stays in the fight past the clock is overrun. It also
   times the adds a boss calls (boss_scripts.cpp): one left alive too
   long merges back into the boss and heals it. */

#define BOSS_MAX_TIMED_ADDS 8

enum boss_clock_stage
{
    BossClock_None,
    BossClock_Running,
    // NOTE(zoubir): under BOSS_CLOCK_WARNING seconds left
    BossClock_Warned,
    // NOTE(zoubir): the clock ran out; the boss hits harder and harder
    BossClock_Enraged,
};

struct boss_clock
{
    // NOTE(zoubir): the boss the clock times, as MonsterSerial, 0 for
    // none; a new serial (a new fight, or the same one after a wipe)
    // starts it again
    u32 BossSerial;
    u32 Stage;
    // NOTE(zoubir): Run->Seconds when the fight started, and how long
    // the boss gives the party
    float StartSeconds;
    float Limit;
    // NOTE(zoubir): what the boss's hits are multiplied by, 1 until it
    // enrages
    float DamageScale;
    // NOTE(zoubir): Doom pulses dealt since the boss enraged
    u32 DoomPulses;
    // NOTE(zoubir): what the HUD shows: whole seconds left (0 once
    // enraged) and the stage. Offline the clock sets them; online the
    // snapshot does (client/dungeon/dungeon_net.cpp)
    u32 ShownSecondsLeft;
    u32 ShownStage;
    // NOTE(zoubir): adds that merge into the boss at Run->Seconds
    // AddDeadline, healing it AddHeal of its health; slot and
    // MonsterSerial as the run keeps its foes
    u32 AddCount;
    u32 AddSlots[BOSS_MAX_TIMED_ADDS];
    u32 AddSerials[BOSS_MAX_TIMED_ADDS];
    float AddDeadline[BOSS_MAX_TIMED_ADDS];
    float AddHeal[BOSS_MAX_TIMED_ADDS];
};

// NOTE(zoubir): from DungeonScaleDamage, for a hit on a player: the
// run's FightingRoom and BossSerial, so the scale ends with the fight
inline float
BossClockDamageScale(boss_clock *Clock, u32 FightingRoom, u32 BossSerial)
{
    float Result = 1.f;
    if (FightingRoom && BossSerial && BossSerial == Clock->BossSerial &&
        Clock->Stage == BossClock_Enraged)
    {
        Result = Clock->DamageScale;
    }
    return Result;
}
