/* Ranger (role_kits/ranger.cpp), the part sim/player.h needs before the
   rest (through sim/dungeon/class_states.h): its two spells with a cast,
   rows PlayerSpell_RangerA and PlayerSpell_RangerB of PlayerSpells
   (sim/player_casts.cpp); what it keeps per player (player_slot.Ranger);
   and what it keeps per run (dungeon_run.Ranger). */

// NOTE(zoubir): {cast seconds, share of the walk left while it casts,
// hover, name over the cast bar}
#define RANGER_CAST_A {1.f, 0.7f, false, "Ranger A"}
#define RANGER_CAST_B {1.f, 0.7f, false, "Ranger B"}

struct ranger_slot
{
    float Unused;
};

struct ranger_run
{
    float Unused;
};
