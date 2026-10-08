/* Frost Mage (role_kits/frostmage.cpp), the part sim/player.h needs before the
   rest (through sim/dungeon/class_states.h): its two spells with a cast,
   rows PlayerSpell_FrostMageA and PlayerSpell_FrostMageB of PlayerSpells
   (sim/player_casts.cpp); what it keeps per player (player_slot.FrostMage);
   and what it keeps per run (dungeon_run.FrostMage). */

// NOTE(zoubir): {cast seconds, share of the walk left while it casts,
// hover, name over the cast bar}
#define FROSTMAGE_CAST_A {1.f, 0.7f, false, "Frost Mage A"}
#define FROSTMAGE_CAST_B {1.f, 0.7f, false, "Frost Mage B"}

struct frostmage_slot
{
    float Unused;
};

struct frostmage_run
{
    u32 Unused;
};
