/* Berserker (role_kits/berserker.cpp), the part sim/player.h needs before the
   rest (through sim/dungeon/class_states.h): its two spells with a cast,
   rows PlayerSpell_BerserkerA and PlayerSpell_BerserkerB of PlayerSpells
   (sim/player_casts.cpp); what it keeps per player (player_slot.Berserker);
   and what it keeps per run (dungeon_run.Berserker). */

// NOTE(zoubir): {cast seconds, share of the walk left while it casts,
// hover, name over the cast bar}
#define BERSERKER_CAST_A {1.f, 0.7f, false, "Berserker A"}
#define BERSERKER_CAST_B {1.f, 0.7f, false, "Berserker B"}

struct berserker_slot
{
    float Unused;
};

struct berserker_run
{
    float Unused;
};
