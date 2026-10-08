/* Duelist (role_kits/duelist.cpp), the part sim/player.h needs before the
   rest (through sim/dungeon/class_states.h): its two spells with a cast,
   rows PlayerSpell_DuelistA and PlayerSpell_DuelistB of PlayerSpells
   (sim/player_casts.cpp); what it keeps per player (player_slot.Duelist);
   and what it keeps per run (dungeon_run.Duelist). A stub until the kit
   lands: nothing here is read yet. */

// NOTE(zoubir): {cast seconds, share of the walk left while it casts,
// hover, name over the cast bar}
#define DUELIST_CAST_A {1.f, 0.7f, false, "Duelist A"}
#define DUELIST_CAST_B {1.f, 0.7f, false, "Duelist B"}

struct duelist_slot
{
    float Unused;
};

struct duelist_run
{
    float Unused;
};
