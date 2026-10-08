/* Stormcaller (role_kits/stormcaller.cpp), the part sim/player.h needs before the
   rest (through sim/dungeon/class_states.h): its two spells with a cast,
   rows PlayerSpell_StormcallerA and PlayerSpell_StormcallerB of PlayerSpells
   (sim/player_casts.cpp); what it keeps per player (player_slot.Stormcaller);
   and what it keeps per run (dungeon_run.Stormcaller).

   Charge lives in stormcaller_slot.Charge and goes to every client as
   ClassMeter (0..100); what the looks and the HUD need goes as ClassFlags
   (STORMCALLER_FLAG_*). */

// NOTE(zoubir): {cast seconds, share of the walk left while it casts,
// hover, name over the cast bar}. Chain Lightning gathers the bolt
// between the hands; Thunderclap raises them to the sky and nearly stands
// still while the cloud answers
#define STORMCALLER_CAST_A {0.6f, 0.7f, false, "Chain Lightning"}
#define STORMCALLER_CAST_B {0.5f, 0.4f, false, "Thunderclap"}

// NOTE(zoubir): player_slot.ClassFlags bits, which every client gets
// (client/dungeon/classes/stormcaller.cpp, ui/dungeon/classes/stormcaller_hud.cpp)
#define STORMCALLER_FLAG_SUPERCHARGED 0x1 // Charge at STORM_SUPERCHARGED or more, or Eye of the Storm
#define STORMCALLER_FLAG_EYE 0x2          // Eye of the Storm is up
#define STORMCALLER_FLAG_GROUNDED 0x4     // just overloaded: smoking, keys held back
#define STORMCALLER_FLAG_FIELD 0x8        // a Static Field of its own is on the ground
#define STORMCALLER_FLAG_EMPTY 0x10       // Thunderclap was pressed short of Charge (the HUD shakes)

struct stormcaller_slot
{
    // NOTE(zoubir): 0..STORM_CHARGE_MOST, sent as ClassMeter
    float Charge;
    // NOTE(zoubir): seconds since the Stormcaller last dealt a hit;
    // Charge drains once it is long
    float IdleSeconds;
    // NOTE(zoubir): seconds left of Eye of the Storm, and to its next bolt
    float EyeSeconds;
    float EyeTimer;
    // NOTE(zoubir): seconds left of the overload's smoke (the flag only;
    // the keys it held back are their own cooldowns)
    float GroundedSeconds;
    // NOTE(zoubir): seconds left of the refusal's flag, and until a
    // refused Thunderclap says so again (a held key retries each tick)
    float EmptySeconds;
    // NOTE(zoubir): the foe a wind-up was pressed on (entity slot and
    // serial), struck when it ends, as Eviscerate's is
    u32 LockSlot;
    u32 LockSerial;
    // NOTE(zoubir): a bot's (server/bots/stormcaller.cpp): how long it has
    // waited for its tank to pull
    float BotWaited;
    // NOTE(zoubir): drawn only (ui/dungeon/classes/stormcaller_hud.cpp,
    // client/dungeon/classes/stormcaller.cpp): the Charge bar's eased
    // fill and the clock it was last drawn at; the simulation never
    // reads them
    float HudCharge;
    float HudClock;
};

// NOTE(zoubir): a Static Field on the ground: every foe inside is struck
// each STATIC_FIELD_TICK, and lightning landing on a foe inside arcs to
// the rest of it
struct stormcaller_field
{
    v3 Position;
    float Radius;
    float Seconds;
    float TickTimer;
    float TickDamage;
    u8 By;
};

#define STORMCALLER_MAX_FIELDS 8

struct stormcaller_run
{
    stormcaller_field Fields[STORMCALLER_MAX_FIELDS];
};
