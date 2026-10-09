/* Boss departures (boss_departures.cpp), the part dungeon.cpp needs
   before the rest: the state a dungeon_run keeps while a Starless Deep
   boss is away from its fight, and the height that puts a monster out
   of the fight and out of sight. */

// NOTE(zoubir): a monster held this far over the floor is out of the
// fight: no blow reaches it, nothing bumps into it, and clients do not
// draw it (client/draw_entities.cpp). A boss that left is held at
// BOSS_AWAY_HEIGHT; the hazards that fall while it is gone hang just
// past the line (sim/monsters/starless_departure_hazards.cpp). Both stay
// well inside what a snapshot can send of a height (net/protocol.h)
#define OUT_OF_SIGHT_HEIGHT 600.f
#define BOSS_AWAY_HEIGHT 1200.f

#define MAX_DEPARTURE_HAZARDS 24

struct boss_departure
{
    // NOTE(zoubir): the boss the fired bits are for, as MonsterSerial; a
    // new serial (a new fight, or the same one after a wipe) clears them
    u32 BossSerial;
    // NOTE(zoubir): one bit per BossDepartures row of that boss, by the
    // row's place among its rows
    u32 Fired;
    // NOTE(zoubir): Run->Seconds when the boss comes back, 0 while it is
    // in the fight; when the next hazards fall; Run->Seconds last tick,
    // for pausing the boss clock
    float BackAt;
    float NextWave;
    float LastSeconds;
    u32 HazardKind;
    // NOTE(zoubir): the hazards falling, as slot and MonsterSerial, and
    // Run->Seconds when each is taken away
    u32 HazardCount;
    u32 HazardSlots[MAX_DEPARTURE_HAZARDS];
    u32 HazardSerials[MAX_DEPARTURE_HAZARDS];
    float HazardGone[MAX_DEPARTURE_HAZARDS];
};
