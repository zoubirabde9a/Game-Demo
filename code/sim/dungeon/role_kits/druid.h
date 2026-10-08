/* Druid (role_kits/druid.cpp), the part sim/player.h needs before the
   rest (through sim/dungeon/class_states.h): its two spells with a cast,
   rows PlayerSpell_DruidA and PlayerSpell_DruidB of PlayerSpells
   (sim/player_casts.cpp); what it keeps per player (player_slot.Druid);
   and what it keeps per run (dungeon_run.Druid). */

// NOTE(zoubir): {cast seconds, share of the walk left while it casts,
// hover, name over the cast bar}
#define DRUID_CAST_A {1.f, 0.7f, false, "Druid A"}
#define DRUID_CAST_B {1.f, 0.7f, false, "Druid B"}

struct druid_slot
{
    float Unused;
};

struct druid_run
{
    u32 Unused;
};
