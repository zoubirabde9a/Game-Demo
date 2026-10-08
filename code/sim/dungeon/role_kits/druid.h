/* Druid (role_kits/druid.cpp), the part sim/player.h needs before the
   rest (through sim/dungeon/class_states.h): its two spells with a cast,
   rows PlayerSpell_DruidA and PlayerSpell_DruidB of PlayerSpells
   (sim/player_casts.cpp); what it keeps per player (player_slot.Druid);
   and what it keeps per run (dungeon_run.Druid). */

// NOTE(zoubir): {cast seconds, share of the walk left while it casts,
// hover, name over the cast bar}. Starfire is a plain cast; Tranquility
// is a channel, its heals going out all through it (UpdateDruidEffects),
// so the walk slows more
#define DRUID_CAST_A {1.5f, 0.7f, false, "Starfire"}
#define DRUID_CAST_B {3.f, 0.4f, false, "Tranquility"}

// NOTE(zoubir): player_slot.ClassFlags bits, which every client gets
// (ui/dungeon/classes/druid_hud.cpp, client/dungeon/classes/druid.cpp)
#define DRUID_FLAG_BLOOM_FULL 1   // Bloom is full: the next heal spends five
#define DRUID_FLAG_TRANQUILITY 2  // Tranquility is channeling
#define DRUID_FLAG_REJUVENATION 4 // a Rejuvenation of this Druid's is healing someone
#define DRUID_FLAG_MOONFIRE 8     // a Moonfire of this Druid's burns a foe
#define DRUID_FLAG_ROOTS 16       // Entangling Roots of this Druid's hold the ground

struct druid_slot
{
    // NOTE(zoubir): 0..DRUID_BLOOM_MOST, sent as ClassMeter
    u32 Bloom;
    // NOTE(zoubir): seconds before Bloom starts fading out of a fight, and
    // the time to the next one fading
    float BloomHold;
    float BloomFade;
    // NOTE(zoubir): Tranquility's time to its next heal
    float TranquilityTimer;
    // NOTE(zoubir): which druid_shot a hit about to land is, for
    // OnDruidHit and DruidDealtScale (DruidShot_None for anything else)
    u32 Hitting;
    // NOTE(zoubir): drawn only (ui/dungeon/classes/druid_hud.cpp): the
    // Bloom bar's eased fill, the clock it was last drawn at, and when it
    // last came full, on the client; the simulation never reads them
    float HudBloom;
    float HudClock;
    float HudFullAt;
};

// NOTE(zoubir): a bolt or a falling star on its way: its hit lands when
// it gets there
struct druid_bolt
{
    u32 TargetSlot;
    u32 TargetSerial;
    float Delay;
    float Damage;
    v2 Away;
    u8 By;
    u8 Shot;
};

// NOTE(zoubir): a Moonfire burning a foe
struct druid_moonfire
{
    u32 TargetSlot;
    u32 TargetSerial;
    float Seconds;
    float TickTimer;
    float Keep;
    u8 By;
};

// NOTE(zoubir): a Rejuvenation healing the player in slot Ally
struct druid_rejuvenation
{
    float Seconds;
    float PerSecond;
    float Keep;
    u8 Ally;
    u8 By;
};

// NOTE(zoubir): Entangling Roots holding a circle of ground
struct druid_roots
{
    v3 Position;
    float Radius;
    float Seconds;
    float Hold;
    float TickTimer;
    u8 By;
};

#define DRUID_MAX_BOLTS 32
#define DRUID_MAX_MOONFIRES 16
#define DRUID_MAX_REJUVENATIONS 16
#define DRUID_MAX_ROOTS 8

struct druid_run
{
    druid_bolt Bolts[DRUID_MAX_BOLTS];
    druid_moonfire Moonfires[DRUID_MAX_MOONFIRES];
    druid_rejuvenation Rejuvenations[DRUID_MAX_REJUVENATIONS];
    druid_roots Roots[DRUID_MAX_ROOTS];
};
