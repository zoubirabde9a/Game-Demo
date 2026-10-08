/* Ranger (role_kits/ranger.cpp), the part sim/player.h needs before the
   rest (through sim/dungeon/class_states.h): its two spells with a cast,
   rows PlayerSpell_RangerA and PlayerSpell_RangerB of PlayerSpells
   (sim/player_casts.cpp); what it keeps per player (player_slot.Ranger);
   and what it keeps per run (dungeon_run.Ranger). */

// NOTE(zoubir): {cast seconds, share of the walk left while it casts,
// hover, name over the cast bar}. Piercing Shot draws the bow for a
// second; Rapid Fire is a channel, its arrows loosed all through the
// cast (UpdateRangerEffects), so the walk slows more
#define RANGER_CAST_A {1.f, 0.7f, false, "Piercing Shot"}
#define RANGER_CAST_B {2.f, 0.6f, false, "Rapid Fire"}

// NOTE(zoubir): player_slot.ClassFlags bits, which every client gets
// (ui/dungeon/classes/ranger_hud.cpp, client/dungeon/classes/ranger.cpp)
#define RANGER_FLAG_MARK 1        // a Hunter's Mark is up on a foe
#define RANGER_FLAG_TRAP 2        // a snare trap waits on the ground
#define RANGER_FLAG_DEADEYE 4     // Focus full with Deadeye: the next Piercing Shot crits
#define RANGER_FLAG_RAPID 8       // Rapid Fire is loosing arrows

struct ranger_slot
{
    // NOTE(zoubir): 0..RANGER_FOCUS_MOST, sent as ClassMeter
    float Focus;
    // NOTE(zoubir): seconds before Focus starts draining out of a fight
    float FocusHold;
    // NOTE(zoubir): the foe under Hunter's Mark (entity slot and serial)
    // and the seconds it has left
    u32 MarkSlot;
    u32 MarkSerial;
    float MarkSeconds;
    // NOTE(zoubir): seconds until the mark's look is sent again
    float MarkKeep;
    // NOTE(zoubir): Rapid Fire's foe, and the time to its next arrow
    u32 RapidSlot;
    u32 RapidSerial;
    float RapidTimer;
    // NOTE(zoubir): which ranger_shot a hit about to land is, for
    // OnRangerHit (RangerShot_None for anything else)
    u32 Hitting;
    // NOTE(zoubir): a bot's (server/bots/ranger.cpp): how long it has
    // waited for its tank to pull
    float BotWaited;
    // NOTE(zoubir): drawn only (ui/dungeon/classes/ranger_hud.cpp): the
    // Focus bar's eased fill, the clock it was last drawn at, and when it
    // last came full, on the client; the simulation never reads them
    float HudFocus;
    float HudClock;
    float HudFullAt;
};

// NOTE(zoubir): an arrow in flight: its hit lands when it gets there
struct ranger_arrow
{
    u32 TargetSlot;
    u32 TargetSerial;
    float Delay;
    float Damage;
    v2 Away;
    u8 By;
    u8 Shot;
};

// NOTE(zoubir): a Volley's circle: Delay while the arrows go up, then
// the rain for Seconds
struct ranger_volley
{
    v3 Position;
    float Radius;
    float Delay;
    float Seconds;
    float TickTimer;
    u8 By;
};

// NOTE(zoubir): a snare trap waiting on the ground
struct ranger_trap
{
    v3 Position;
    float Seconds;
    float Keep;
    u8 By;
};

#define RANGER_MAX_ARROWS 48
#define RANGER_MAX_VOLLEYS 8
#define RANGER_MAX_TRAPS 8

struct ranger_run
{
    ranger_arrow Arrows[RANGER_MAX_ARROWS];
    ranger_volley Volleys[RANGER_MAX_VOLLEYS];
    ranger_trap Traps[RANGER_MAX_TRAPS];
};
