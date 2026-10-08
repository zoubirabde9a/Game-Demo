/* Shadowblade (role_kits/shadowblade.cpp), the part sim/player.h needs before the
   rest (through sim/dungeon/class_states.h): its two spells with a cast,
   rows PlayerSpell_ShadowbladeA and PlayerSpell_ShadowbladeB of PlayerSpells
   (sim/player_casts.cpp); what it keeps per player (player_slot.Shadowblade);
   and what it keeps per run (dungeon_run.Shadowblade). */

// NOTE(zoubir): {cast seconds, share of the walk left while it casts,
// hover, name over the cast bar}
#define SHADOWBLADE_CAST_A {1.f, 0.7f, false, "Shadowblade A"}
#define SHADOWBLADE_CAST_B {1.f, 0.7f, false, "Shadowblade B"}

struct shadowblade_slot
{
    float Unused;
};

struct shadowblade_run
{
    float Unused;
};
