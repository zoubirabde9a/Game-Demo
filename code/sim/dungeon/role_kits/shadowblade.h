/* Shadowblade (role_kits/shadowblade.cpp), the part sim/player.h needs before the
   rest (through sim/dungeon/class_states.h): its two spells with a cast,
   rows PlayerSpell_ShadowbladeA and PlayerSpell_ShadowbladeB of PlayerSpells
   (sim/player_casts.cpp); what it keeps per player (player_slot.Shadowblade);
   and what it keeps per run (dungeon_run.Shadowblade). */

// NOTE(zoubir): {cast seconds, share of the walk left while it casts,
// hover, name over the cast bar}: Fan of Knives crouches a moment before
// the knives fly, Eviscerate draws both daggers back before the flurry
#define SHADOWBLADE_CAST_A {0.22f, 0.8f, false, "Fan of Knives"}
#define SHADOWBLADE_CAST_B {0.18f, 0.85f, false, "Eviscerate"}

// NOTE(zoubir): player_slot.ClassFlags bits of a Shadowblade, which every
// client gets (the looks and the HUD read them)
#define SHADOWBLADE_FLAG_DANCE 0x1
#define SHADOWBLADE_FLAG_CRIT 0x2
// NOTE(zoubir): the fading combo points are going out of combat
#define SHADOWBLADE_FLAG_FADING 0x4

#define SHADOWBLADE_MOST_POINTS 5

struct shadowblade_slot
{
    // NOTE(zoubir): seconds left of the critical strike Shadowstep gives
    // and of Shadow Dance
    float CritSeconds;
    float DanceSeconds;
    // NOTE(zoubir): Twin Strike's second cut, landing CutDelay after the
    // first along CutDirection
    float CutDelay;
    v2 CutDirection;
    bool32 CutLanded;
    // NOTE(zoubir): seconds since the Shadowblade last struck or built a
    // point; combo points fade once it is long, and the fade's own clock
    float IdleSeconds;
    float FadeTimer;
};

// NOTE(zoubir): a Poisoned Shiv's poison on a monster: whose, how long it
// has left and how hard it bites; a free row has Seconds 0
#define SHADOWBLADE_POISONS 24
struct shadowblade_poison
{
    u32 Slot;
    u32 Serial;
    u32 By;
    float Seconds;
    float PerSecond;
    float TickTimer;
};

// NOTE(zoubir): a Smoke Bomb's cloud on the ground; Seconds 0 is a free one
#define SHADOWBLADE_SMOKES 8
struct shadowblade_smoke
{
    v3 Position;
    float Seconds;
    float Radius;
    float Share;
    u32 By;
    bool32 Slows;
};

struct shadowblade_run
{
    shadowblade_poison Poisons[SHADOWBLADE_POISONS];
    shadowblade_smoke Smokes[SHADOWBLADE_SMOKES];
};
