/* Party scaling (dungeon.cpp): how a dungeon fight grows with the size
   of the party. Monster health and damage grow by a fixed factor for
   each player past the first, so every player who joins makes the run
   harder than the one before did. The roles keep up at the square root
   of the extra damage: the tank takes only that much more, heals and
   wards grow by it. StartEncounter (encounters.cpp) applies the health
   and stores the damage on the run; DungeonScaleDamage reads it. */

// NOTE(zoubir): each player past the first multiplies a dungeon
// monster's health by PARTY_HEALTH_GROWTH and its hits by
// PARTY_DAMAGE_GROWTH, so every player who joins makes the run harder than
// the last one did. Past the three the dungeon is made for, each player
// adds only PARTY_DAMAGE_GROWTH_PAST of damage: a bigger party still has
// one healer to cover everyone, and at the full growth four bots wiped
// on the Ashen Causeway six times in a row in under half a minute each.
// Four players face 3.05 times the health and 1.33 times the damage,
// eight 13.5 times and 1.68 times
#define PARTY_HEALTH_GROWTH 1.45f
#define PARTY_DAMAGE_GROWTH 1.12f
#define PARTY_DAMAGE_GROWTH_PAST 1.06f
#define PARTY_TUNED_SIZE 3

// NOTE(zoubir): every dungeon monster, before the party's growth, has
// DUNGEON_FOE_HEALTH of its health and hits DUNGEON_FOE_DAMAGE as hard.
// Players asked for fights that are quick and dangerous rather than long:
// at full health three bots spent up to 150 s on the Ossuary and two
// wiped on most bosses with a tenth to a third of the boss left
#define DUNGEON_FOE_HEALTH 0.6f
#define DUNGEON_FOE_DAMAGE 1.15f

// NOTE(zoubir): while a boss is up, every monster hit on a player is
// DUNGEON_BOSS_DAMAGE times as hard on top of that, the tank's included.
// Playtesters said the bosses did not do much damage: a boss's shots
// carry no link back to it, so the whole fight takes the scale, its
// adds too, as the enrage scale does (boss_clock.h)
#define DUNGEON_BOSS_DAMAGE 1.4f

// NOTE(zoubir): Growth to the power of the players past the first; a
// loop rather than powf, so the ARM server and an x86 machine get the
// same float
inline float
PartyGrowth(float Growth, u32 Players)
{
    float Result = 1.f;
    for(u32 Extra = 1; Extra < Players; Extra++)
    {
        Result *= Growth;
    }
    return Result;
}

inline float
PartyHealthScale(u32 Players)
{
    float Result = PartyGrowth(PARTY_HEALTH_GROWTH, Players);
    return Result;
}

inline float
PartyDamageScale(u32 Players)
{
    u32 Tuned = Minimum(Players, (u32)PARTY_TUNED_SIZE);
    float Result = PartyGrowth(PARTY_DAMAGE_GROWTH, Tuned);
    for(u32 Extra = Tuned; Extra < Players; Extra++)
    {
        Result *= PARTY_DAMAGE_GROWTH_PAST;
    }
    return Result;
}

// NOTE(zoubir): what the monsters' hits are multiplied by in the room
// being fought (StartEncounter sets it for the party's size)
inline float
RunPartyDamage(dungeon_run *Run)
{
    float Result = (Run && Run->PartyDamage > 0.f) ? Run->PartyDamage : 1.f;
    return Result;
}

// NOTE(zoubir): what a monster's hit on a player is multiplied by for
// the boss: DUNGEON_BOSS_DAMAGE while one is being fought, else 1
inline float
RunBossDamage(dungeon_run *Run)
{
    float Result = (Run && Run->FightingRoom && Run->BossSerial) ? DUNGEON_BOSS_DAMAGE : 1.f;
    return Result;
}

// NOTE(zoubir): the roles keep up with a bigger party at the square root
// of its damage: a tank takes only that much more, and heals and wards
// grow by it, so eight players' hits (2.2 times) land on the tank 1.49
// times as hard and a healer's spells are 1.49 times as strong
inline float
PartySustainScale(dungeon_run *Run)
{
    float Result = SquareRoot(RunPartyDamage(Run));
    return Result;
}
