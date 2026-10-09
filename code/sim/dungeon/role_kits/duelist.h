/* Duelist (role_kits/duelist.cpp), the part sim/player.h needs before the
   rest (through sim/dungeon/class_states.h): its two spells with a cast,
   rows PlayerSpell_DuelistA and PlayerSpell_DuelistB of PlayerSpells
   (sim/player_casts.cpp); what it keeps per player (player_slot.Duelist);
   and what it keeps per run (dungeon_run.Duelist, nothing yet).

   Tempo lives in player_slot.ClassMeter (0..DUELIST_MOST_TEMPO), and what
   the looks need in ClassFlags (DUELIST_FLAG_*), so both reach every
   client. */

// NOTE(zoubir): {cast seconds, share of the walk left while it casts,
// hover, name over the cast bar}. Riposte's guard is a timer of the
// slot's, not a cast (a cast cannot be cut short by the parry, and Bait
// lengthens the guard), so A is unused. Heartseeker draws the point back
// and nearly stands still before it lunges through
#define DUELIST_CAST_A {1.f, 0.7f, false, "Duelist A"}
#define DUELIST_CAST_B {0.35f, 0.35f, false, "Heartseeker"}

// NOTE(zoubir): player_slot.ClassFlags bits of a Duelist, which every
// client gets (the looks and the HUD read them): on guard (Riposte), the
// guard has parried a hit already, Tempo at its top (the blade glows),
// Perfect Form up, and Tempo fading out of a fight
#define DUELIST_FLAG_GUARD 0x1
#define DUELIST_FLAG_PARRIED 0x2
#define DUELIST_FLAG_TEMPO 0x4
#define DUELIST_FLAG_FORM 0x8
#define DUELIST_FLAG_FADING 0x10
// NOTE(zoubir): a Feint's dodge is up
#define DUELIST_FLAG_FEINT 0x20

#define DUELIST_MOST_TEMPO 5

struct duelist_slot
{
    // NOTE(zoubir): the role key the Duelist used last (RoleKeys order)
    // + 1, 0 for none: a spell gains Tempo only on another key
    u32 LastKey;
    // NOTE(zoubir): Riposte's guard: seconds left, and whether it has
    // parried a hit yet (one counter a guard)
    float GuardSeconds;
    bool32 Parried;
    // NOTE(zoubir): seconds left of a Feint's dodge
    float FeintSeconds;
    // NOTE(zoubir): a counter owed by a parry, struck on the next tick
    // (UpdateDuelistEffects) rather than inside the hit that set it off:
    // the foe it goes for first (the attacker, by entity slot and serial)
    bool32 CounterDue;
    u32 CounterSlot;
    u32 CounterSerial;
    // NOTE(zoubir): seconds left of Perfect Form
    float FormSeconds;
    // NOTE(zoubir): Perfect Form's second Thrust, landing EchoDelay after
    // the first along EchoDirection
    float EchoDelay;
    v2 EchoDirection;
    // NOTE(zoubir): the foe Heartseeker was pressed on (its entity slot
    // and serial), struck when the wind-up ends, and whether the press
    // was on a new key (so the strike gains Tempo)
    u32 HeartSlot;
    u32 HeartSerial;
    bool32 HeartFresh;
    // NOTE(zoubir): Masterstroke's second strike on that foe: seconds
    // until it lands (0 for none) and how hard
    float MasterDelay;
    float MasterDamage;
    // NOTE(zoubir): seconds since the Duelist last dealt a hit (Tempo
    // fades out of a fight once it is long), and the fade's own clock
    float IdleSeconds;
    float FadeTimer;
    // NOTE(zoubir): client only (ui/dungeon/classes/duelist_hud.cpp): the
    // Tempo and flags the HUD last drew, and when it last saw Tempo rise,
    // the guard go up and Perfect Form start (their bursts are gone well
    // before either ends)
    u32 ShownTempo;
    float GainClock;
    u32 ShownFlags;
    float GuardClock;
    float FormClock;
};

struct duelist_run
{
    float Unused;
};
