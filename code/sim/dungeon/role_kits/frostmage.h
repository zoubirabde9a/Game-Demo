/* Frost Mage (role_kits/frostmage.cpp), the part sim/player.h needs before
   the rest (through sim/dungeon/class_states.h): its spells with a cast,
   rows PlayerSpell_FrostMageA and PlayerSpell_FrostMageB of PlayerSpells
   (sim/player_casts.cpp); what it keeps per player (player_slot.FrostMage);
   and what it keeps per run (dungeon_run.FrostMage). */

// NOTE(zoubir): {cast seconds, share of the walk left while it casts,
// hover, name over the cast bar}. Glacial Spike is the class's one
// wind-up; B is named but nothing casts it yet
#define FROSTMAGE_CAST_A {1.25f, 0.6f, false, "Glacial Spike"}
#define FROSTMAGE_CAST_B {1.f, 0.7f, false, "Frost Mage"}

// NOTE(zoubir): player_slot.ClassFlags bits, which every client gets
// (ui/dungeon/classes/frostmage_hud.cpp, client/dungeon/classes/frostmage.cpp)
#define FROSTMAGE_FLAG_BARRIER 1  // Ice Barrier holds
#define FROSTMAGE_FLAG_FINGERS 2  // Fingers of Frost: the next Frostbolt shatters
#define FROSTMAGE_FLAG_ORB 4      // a Frozen Orb of this mage's rolls
#define FROSTMAGE_FLAG_FULL 8     // five Icicles: Glacial Spike will freeze

struct frostmage_slot
{
    // NOTE(zoubir): 0..FROSTMAGE_ICICLES_MOST, sent as ClassMeter
    u32 Icicles;
    // NOTE(zoubir): seconds before Icicles start melting out of a fight,
    // and the time to the next one melting
    float IcicleHold;
    float MeltTimer;
    // NOTE(zoubir): Frostbolts cast, for Fingers of Frost's every fourth
    u32 Bolts;
    // NOTE(zoubir): which frostmage_shot a hit about to land is, and
    // whether it shatters whatever it hits (Fingers of Frost), for
    // OnFrostMageHit and FrostMageDealtScale
    u32 Hitting;
    bool32 Shattering;
    // NOTE(zoubir): a bot's (server/bots/frostmage.cpp): how long it has
    // waited for its tank to pull
    float BotWaited;
    // NOTE(zoubir): drawn only (ui/dungeon/classes/frostmage_hud.cpp): the
    // Icicles the bar shows, eased, and the clock it was last drawn at, on
    // the client; the simulation never reads them
    float HudIcicles;
    float HudClock;
};

// NOTE(zoubir): a bolt or a spike in flight: its hit lands when it gets
// there
struct frostmage_bolt
{
    u32 TargetSlot;
    u32 TargetSerial;
    float Delay;
    float Damage;
    v2 Away;
    u8 By;
    u8 Shot;
    // NOTE(zoubir): Fingers of Frost's shatter; a Glacial Spike's Icicles
    u8 Shatters;
    u8 Icicles;
};

// NOTE(zoubir): a Blizzard's circle, ice falling for Seconds
struct frostmage_blizzard
{
    v3 Position;
    float Radius;
    float Seconds;
    float TickTimer;
    float Chill;
    u8 By;
};

// NOTE(zoubir): a Frozen Orb rolling along Direction for Seconds
struct frostmage_orb
{
    v3 Position;
    v2 Direction;
    float Seconds;
    float TickTimer;
    u8 By;
};

#define FROSTMAGE_MAX_BOLTS 48
#define FROSTMAGE_MAX_BLIZZARDS 8
#define FROSTMAGE_MAX_ORBS 8

struct frostmage_run
{
    frostmage_bolt Bolts[FROSTMAGE_MAX_BOLTS];
    frostmage_blizzard Blizzards[FROSTMAGE_MAX_BLIZZARDS];
    frostmage_orb Orbs[FROSTMAGE_MAX_ORBS];
};
