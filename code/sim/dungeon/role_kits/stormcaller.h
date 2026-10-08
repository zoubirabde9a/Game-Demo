/* Stormcaller (role_kits/stormcaller.cpp), the part sim/player.h needs before the
   rest (through sim/dungeon/class_states.h): its two spells with a cast,
   rows PlayerSpell_StormcallerA and PlayerSpell_StormcallerB of PlayerSpells
   (sim/player_casts.cpp); what it keeps per player (player_slot.Stormcaller);
   and what it keeps per run (dungeon_run.Stormcaller). A stub until the kit
   lands: nothing here is read yet. */

// NOTE(zoubir): {cast seconds, share of the walk left while it casts,
// hover, name over the cast bar}
#define STORMCALLER_CAST_A {1.f, 0.7f, false, "Stormcaller A"}
#define STORMCALLER_CAST_B {1.f, 0.7f, false, "Stormcaller B"}

struct stormcaller_slot
{
    float Unused;
};

struct stormcaller_run
{
    float Unused;
};
