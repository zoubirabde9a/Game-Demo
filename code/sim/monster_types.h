#if !defined(SIM_MONSTER_TYPES_H)
#define SIM_MONSTER_TYPES_H
/* Monster kinds, monster ability phases and status effects: the enums
   and limits world_entity's monster fields (monster_fields.inc) use.
   Included by entity.h. */

// NOTE(zoubir): one MonsterKind_<Name> per line of monster_list.inc, see
// code/sim/monsters/README.md
enum monster_kind
{
#define MONSTER(Name) MonsterKind_##Name,
#define MONSTER_NAME_PASS
#include "monsters/monster_list.inc"
#undef MONSTER_NAME_PASS
#undef MONSTER
    MonsterKind_Count
};

// NOTE(zoubir): timed conditions, see sim/status_effects.cpp
enum status_effect
{
    StatusEffect_None,
    StatusEffect_Burning,
    StatusEffect_Poisoned,
    StatusEffect_Slowed,
    StatusEffect_Stunned,
    StatusEffect_Bleeding,
    StatusEffect_Regenerating,
    StatusEffect_Hasted,
    StatusEffect_Rooted,
    StatusEffect_Soaked,
    StatusEffect_Falling,
    StatusEffect_Count
};

#define MAX_MONSTER_ABILITIES 4
#define MAX_ABILITY_POINTS 4

// NOTE(zoubir): every monster ability runs Windup (rooted, telegraphed,
// can be dodged) -> Active (the hit or the movement) -> Recover (open to
// punishment), then goes back to Ready. See sim/monster_abilities.cpp
enum ability_phase
{
    AbilityPhase_Ready,
    AbilityPhase_Windup,
    AbilityPhase_Active,
    AbilityPhase_Recover,
};

#endif
