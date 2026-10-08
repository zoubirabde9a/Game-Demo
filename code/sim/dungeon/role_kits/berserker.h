/* Berserker (role_kits/berserker.cpp), the part sim/player.h needs before the
   rest (through sim/dungeon/class_states.h): its two spells with a cast,
   rows PlayerSpell_BerserkerA and PlayerSpell_BerserkerB of PlayerSpells
   (sim/player_casts.cpp); what it keeps per player (player_slot.Berserker);
   and what it keeps per run (dungeon_run.Berserker).

   Rage lives in player_slot.ClassMeter (0..100), and what the looks need
   in ClassFlags (BERSERKER_FLAG_*), so both reach every client. */

// NOTE(zoubir): {cast seconds, share of the walk left while it casts,
// hover, name over the cast bar}. Whirlwind (R) spins for its whole cast,
// still walking slowly; Execute (W) raises the axe and holds still
#define BERSERKER_CAST_A {1.5f, 0.55f, false, "Whirlwind"}
#define BERSERKER_CAST_B {0.45f, 0.2f, false, "Execute"}

// NOTE(zoubir): ClassFlags bits: Berserk is up; a spell was pressed
// without the Rage it costs (the HUD flashes); the Berserker is in a
// Leap's flight (the axe goes up)
#define BERSERKER_FLAG_BERSERK 0x1
#define BERSERKER_FLAG_NO_RAGE 0x2
#define BERSERKER_FLAG_LEAPING 0x4

// NOTE(zoubir): the class's bursts, SimBurst_BerserkerFirst + these:
// - Cleave, CleaveBack: the axe's swing round Position (the feet), across
//   Angle, one way round or the other;
// - AxeThrow: a hand axe from the Berserker to Position (the foe's chest)
//   and back, AXE_THROW_SPEED each way;
// - Bloodthirst: a strike on the foe at Position from along Angle, and
//   its blood drawn back into the Berserker;
// - Leap: the jump starting, Position where it lands;
// - Slam: the Leap landing at Position;
// - Execute: the chop landing at Position, along Angle;
// - Berserk: the war cry at Position as Berserk goes up
enum berserker_burst
{
    BerserkerBurst_Cleave,
    BerserkerBurst_CleaveBack,
    BerserkerBurst_AxeThrow,
    BerserkerBurst_Bloodthirst,
    BerserkerBurst_Leap,
    BerserkerBurst_Slam,
    BerserkerBurst_Execute,
    BerserkerBurst_Berserk,
};

// NOTE(zoubir): axes thrown at once across the party
#define MAX_BERSERKER_AXES 8

struct berserker_slot
{
    // NOTE(zoubir): the share of a Rage point not yet whole (Rage is
    // whole points in ClassMeter); seconds since the Berserker last hit or was
    // hit (Rage decays after BERSERKER_CALM_SECONDS); health last tick, for
    // the Rage hits taken give
    float RageCarry;
    float CalmSeconds;
    float LastHp;
    float BerserkSeconds;
    float NoRageSeconds;
    // NOTE(zoubir): the next Cleave swings the other way round
    bool32 CleaveBack;
    // NOTE(zoubir): a Leap in flight: where it lands and the seconds since
    // take-off
    bool32 Leaping;
    v2 LeapTo;
    float LeapAge;
    // NOTE(zoubir): Whirlwind: the spin's hits so far this cast
    u32 WhirlHits;
    // NOTE(zoubir): client only (client/dungeon/classes/berserker.cpp):
    // the burst clock of the newest burst of this player already started
    // (a body swing, cracks), and the flags seen last frame
    float FxSeen;
    u32 FlagsSeen;
};

// NOTE(zoubir): a hand axe in flight to a foe: it lands after Delay
struct berserker_axe
{
    float Delay;
    u32 By;
    u32 Slot;
    u32 Serial;
};

struct berserker_run
{
    berserker_axe Axes[MAX_BERSERKER_AXES];
};
